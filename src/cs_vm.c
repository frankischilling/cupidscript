#include "cs_vm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char* cs_strdup2(const char* s) {
    size_t n = strlen(s ? s : "");
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = 0;
    return p;
}

// Exposed error helpers
const char* cs_vm_last_error(cs_vm* vm) {
    if (!vm || !vm->last_error) return "";
    return vm->last_error;
}

void cs_error(cs_vm* vm, const char* msg) {
    if (!vm) return;
    free(vm->last_error);
    vm->last_error = cs_strdup2(msg ? msg : "error");
}

// ---------- value helpers ----------
cs_value cs_nil(void) { cs_value v; v.type = CS_T_NIL; v.as.p = NULL; return v; }
cs_value cs_bool(int x){ cs_value v; v.type=CS_T_BOOL; v.as.b = !!x; return v; }
cs_value cs_int(int64_t x){ cs_value v; v.type=CS_T_INT; v.as.i = x; return v; }

cs_type cs_typeof(cs_value v) { return v.type; }
const char* cs_type_name(cs_type t){ return cs_type_name_impl(t); }

static cs_native* as_native(cs_value v){ return (cs_native*)v.as.p; }
static cs_string* as_str(cs_value v){ return (cs_string*)v.as.p; }
static struct cs_func* as_func(cs_value v){ return (struct cs_func*)v.as.p; }

const char* cs_to_cstr(cs_value v) {
    if (v.type == CS_T_STR) return as_str(v)->data;
    return NULL;
}

cs_value cs_str(cs_vm* vm, const char* s) {
    (void)vm;
    cs_string* st = cs_str_new(s ? s : "");
    cs_value v; v.type = CS_T_STR; v.as.p = st;
    return v;
}

cs_value cs_value_copy(cs_value v) {
    if (v.type == CS_T_STR) cs_str_incref(as_str(v));
    else if (v.type == CS_T_NATIVE) as_native(v)->ref++;
    else if (v.type == CS_T_FUNC) as_func(v)->ref++;
    return v;
}

void cs_value_release(cs_value v) {
    if (v.type == CS_T_STR) cs_str_decref(as_str(v));
    else if (v.type == CS_T_NATIVE) {
        cs_native* nf = as_native(v);
        if (nf && --nf->ref <= 0) free(nf);
    } else if (v.type == CS_T_FUNC) {
        struct cs_func* f = as_func(v);
        if (f && --f->ref <= 0) free(f);
    }
}

// ---------- env ----------
static cs_env* env_new(cs_env* parent) {
    cs_env* e = (cs_env*)calloc(1, sizeof(cs_env));
    if (!e) return NULL;
    e->parent = parent;
    e->cap = 16;
    e->keys = (char**)calloc(e->cap, sizeof(char*));
    e->vals = (cs_value*)calloc(e->cap, sizeof(cs_value));
    return e;
}

static void env_free(cs_env* e) {
    if (!e) return;
    for (size_t i = 0; i < e->count; i++) {
        free(e->keys[i]);
        cs_value_release(e->vals[i]);
    }
    free(e->keys);
    free(e->vals);
    free(e);
}

static int env_find(cs_env* e, const char* key) {
    for (size_t i = 0; i < e->count; i++) {
        if (strcmp(e->keys[i], key) == 0) return (int)i;
    }
    return -1;
}

static void env_set_here(cs_env* e, const char* key, cs_value v) {
    int idx = env_find(e, key);
    if (idx >= 0) {
        cs_value_release(e->vals[idx]);
        e->vals[idx] = cs_value_copy(v);
        return;
    }
    if (e->count == e->cap) {
        e->cap *= 2;
        e->keys = (char**)realloc(e->keys, e->cap * sizeof(char*));
        e->vals = (cs_value*)realloc(e->vals, e->cap * sizeof(cs_value));
    }
    e->keys[e->count] = cs_strdup2(key);
    e->vals[e->count] = cs_value_copy(v);
    e->count++;
}

static int env_set(cs_env* e, const char* key, cs_value v) {
    // assign walks upward, but will set in nearest existing scope; otherwise in current scope
    for (cs_env* cur = e; cur; cur = cur->parent) {
        int idx = env_find(cur, key);
        if (idx >= 0) {
            cs_value_release(cur->vals[idx]);
            cur->vals[idx] = cs_value_copy(v);
            return 1;
        }
    }
    env_set_here(e, key, v);
    return 1;
}

static int env_get(cs_env* e, const char* key, cs_value* out) {
    for (cs_env* cur = e; cur; cur = cur->parent) {
        int idx = env_find(cur, key);
        if (idx >= 0) { *out = cs_value_copy(cur->vals[idx]); return 1; }
    }
    return 0;
}

static int is_truthy(cs_value v) {
    switch (v.type) {
        case CS_T_NIL: return 0;
        case CS_T_BOOL: return v.as.b != 0;
        default: return 1;
    }
}

// ---------- error ----------
static void vm_set_err(cs_vm* vm, const char* msg, int line, int col) {
    free(vm->last_error);
    char buf[512];
    snprintf(buf, sizeof(buf), "Runtime error at %d:%d: %s", line, col, msg);
    vm->last_error = cs_strdup2(buf);
}

const char* cs_last_error(const cs_vm* vm) {
    return vm ? vm->last_error : NULL;
}

// ---------- string unescape ----------
static char* unescape_string_token(const char* tok, size_t n) {
    // tok includes quotes; minimal unescape
    if (n < 2 || tok[0] != '"' || tok[n-1] != '"') return cs_strdup2("");
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    size_t w = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        char c = tok[i];
        if (c == '\\' && i + 1 < n - 1) {
            char x = tok[++i];
            switch (x) {
                case 'n': out[w++] = '\n'; break;
                case 't': out[w++] = '\t'; break;
                case 'r': out[w++] = '\r'; break;
                case '"': out[w++] = '"'; break;
                case '\\': out[w++] = '\\'; break;
                default: out[w++] = x; break;
            }
        } else {
            out[w++] = c;
        }
    }
    out[w] = 0;
    return out;
}

// ---------- eval ----------
typedef struct {
    int did_return;
    cs_value ret;
    int ok;
} exec_result;

static exec_result exec_stmt(cs_vm* vm, cs_env* env, ast* s);
static exec_result exec_block(cs_vm* vm, cs_env* env, ast* b);
static cs_value eval_expr(cs_vm* vm, cs_env* env, ast* e, int* ok);

static cs_value eval_binop(cs_vm* vm, ast* e, cs_value a, cs_value b, int* ok) {
    int op = e->as.binop.op;

    // short-circuit done in caller for && ||
    if (op == TK_PLUS) {
        if (a.type == CS_T_INT && b.type == CS_T_INT) return cs_int(a.as.i + b.as.i);
        if (a.type == CS_T_STR || b.type == CS_T_STR) {
            const char* sa = (a.type == CS_T_STR) ? as_str(a)->data : NULL;
            const char* sb = (b.type == CS_T_STR) ? as_str(b)->data : NULL;

            char bufA[64], bufB[64];
            if (!sa) { snprintf(bufA, sizeof(bufA), "%lld", (long long)a.as.i); sa = bufA; }
            if (!sb) { snprintf(bufB, sizeof(bufB), "%lld", (long long)b.as.i); sb = bufB; }

            size_t na = strlen(sa), nb = strlen(sb);
            char* joined = (char*)malloc(na + nb + 1);
            if (!joined) { vm_set_err(vm, "out of memory", e->line, e->col); *ok = 0; return cs_nil(); }
            memcpy(joined, sa, na);
            memcpy(joined + na, sb, nb + 1);
            cs_value out = cs_str(vm, joined);
            free(joined);
            return out;
        }
        vm_set_err(vm, "type error: '+' expects int or string", e->line, e->col);
        *ok = 0; return cs_nil();
    }

    if (op == TK_MINUS || op == TK_STAR || op == TK_SLASH || op == TK_PERCENT) {
        if (a.type != CS_T_INT || b.type != CS_T_INT) {
            vm_set_err(vm, "type error: arithmetic expects int", e->line, e->col);
            *ok = 0; return cs_nil();
        }
        if (op == TK_MINUS) return cs_int(a.as.i - b.as.i);
        if (op == TK_STAR) return cs_int(a.as.i * b.as.i);
        if (op == TK_SLASH) {
            if (b.as.i == 0) { vm_set_err(vm, "division by zero", e->line, e->col); *ok=0; return cs_nil(); }
            return cs_int(a.as.i / b.as.i);
        }
        if (op == TK_PERCENT) {
            if (b.as.i == 0) { vm_set_err(vm, "mod by zero", e->line, e->col); *ok=0; return cs_nil(); }
            return cs_int(a.as.i % b.as.i);
        }
    }

    if (op == TK_EQ || op == TK_NE) {
        int eq = 0;
        if (a.type != b.type) eq = 0;
        else if (a.type == CS_T_NIL) eq = 1;
        else if (a.type == CS_T_BOOL) eq = (a.as.b == b.as.b);
        else if (a.type == CS_T_INT) eq = (a.as.i == b.as.i);
        else if (a.type == CS_T_STR) eq = (strcmp(as_str(a)->data, as_str(b)->data) == 0);
        else eq = (a.as.p == b.as.p);

        return cs_bool((op == TK_EQ) ? eq : !eq);
    }

    if (op == TK_LT || op == TK_LE || op == TK_GT || op == TK_GE) {
        if (a.type == CS_T_INT && b.type == CS_T_INT) {
            int r = 0;
            if (op == TK_LT) r = (a.as.i < b.as.i);
            if (op == TK_LE) r = (a.as.i <= b.as.i);
            if (op == TK_GT) r = (a.as.i > b.as.i);
            if (op == TK_GE) r = (a.as.i >= b.as.i);
            return cs_bool(r);
        }
        if (a.type == CS_T_STR && b.type == CS_T_STR) {
            int c = strcmp(as_str(a)->data, as_str(b)->data);
            int r = 0;
            if (op == TK_LT) r = (c < 0);
            if (op == TK_LE) r = (c <= 0);
            if (op == TK_GT) r = (c > 0);
            if (op == TK_GE) r = (c >= 0);
            return cs_bool(r);
        }
        vm_set_err(vm, "type error: comparisons require both ints or both strings", e->line, e->col);
        *ok = 0; return cs_nil();
    }

    vm_set_err(vm, "unknown binary operator", e->line, e->col);
    *ok = 0; return cs_nil();
}

static cs_value eval_expr(cs_vm* vm, cs_env* env, ast* e, int* ok) {
    if (!e) return cs_nil();

    switch (e->type) {
        case N_LIT_INT: return cs_int((int64_t)e->as.lit_int.v);
        case N_LIT_BOOL: return cs_bool(e->as.lit_bool.v);
        case N_LIT_NIL: return cs_nil();

        case N_LIT_STR: {
            size_t n = strlen(e->as.lit_str.s);
            char* un = unescape_string_token(e->as.lit_str.s, n);
            if (!un) { vm_set_err(vm, "out of memory", e->line, e->col); *ok = 0; return cs_nil(); }
            cs_value v = cs_str(vm, un);
            free(un);
            return v;
        }

        case N_IDENT: {
            cs_value v;
            if (!env_get(env, e->as.ident.name, &v)) {
                vm_set_err(vm, "undefined variable", e->line, e->col);
                *ok = 0; return cs_nil();
            }
            return v;
        }

        case N_UNOP: {
            cs_value x = eval_expr(vm, env, e->as.unop.expr, ok);
            if (!*ok) return cs_nil();
            if (e->as.unop.op == TK_BANG) { cs_value out = cs_bool(!is_truthy(x)); cs_value_release(x); return out; }
            if (e->as.unop.op == TK_MINUS) {
                if (x.type != CS_T_INT) { cs_value_release(x); vm_set_err(vm, "type error: unary '-' expects int", e->line, e->col); *ok=0; return cs_nil(); }
                cs_value out = cs_int(-x.as.i);
                cs_value_release(x);
                return out;
            }
            cs_value_release(x);
            vm_set_err(vm, "unknown unary operator", e->line, e->col);
            *ok=0; return cs_nil();
        }

        case N_BINOP: {
            int op = e->as.binop.op;

            // short-circuit
            if (op == TK_ANDAND) {
                cs_value left = eval_expr(vm, env, e->as.binop.left, ok);
                if (!*ok) return cs_nil();
                int lt = is_truthy(left);
                cs_value_release(left);
                if (!lt) return cs_bool(0);
                cs_value right = eval_expr(vm, env, e->as.binop.right, ok);
                if (!*ok) return cs_nil();
                int rt = is_truthy(right);
                cs_value_release(right);
                return cs_bool(rt);
            }
            if (op == TK_OROR) {
                cs_value left = eval_expr(vm, env, e->as.binop.left, ok);
                if (!*ok) return cs_nil();
                int lt = is_truthy(left);
                cs_value_release(left);
                if (lt) return cs_bool(1);
                cs_value right = eval_expr(vm, env, e->as.binop.right, ok);
                if (!*ok) return cs_nil();
                int rt = is_truthy(right);
                cs_value_release(right);
                return cs_bool(rt);
            }

            cs_value a = eval_expr(vm, env, e->as.binop.left, ok);
            if (!*ok) return cs_nil();
            cs_value b = eval_expr(vm, env, e->as.binop.right, ok);
            if (!*ok) { cs_value_release(a); return cs_nil(); }

            cs_value out = eval_binop(vm, e, a, b, ok);
            cs_value_release(a);
            cs_value_release(b);
            return out;
        }

        case N_CALL: {
            cs_value callee = eval_expr(vm, env, e->as.call.callee, ok);
            if (!*ok) return cs_nil();

            int argc = (int)e->as.call.argc;
            cs_value* argv = NULL;
            if (argc > 0) {
                argv = (cs_value*)calloc((size_t)argc, sizeof(cs_value));
                if (!argv) { cs_value_release(callee); vm_set_err(vm, "out of memory", e->line, e->col); *ok = 0; return cs_nil(); }
                for (int i = 0; i < argc; i++) {
                    argv[i] = eval_expr(vm, env, e->as.call.args[i], ok);
                    if (!*ok) break;
                }
            }

            cs_value out = cs_nil();

            if (*ok) {
                if (callee.type == CS_T_NATIVE) {
                    cs_native* nf = as_native(callee);
                    if (nf->fn(vm, nf->userdata, argc, argv, &out) != 0) {
                        // Preserve any existing native-provided error message
                        if (!vm->last_error) vm_set_err(vm, "native call failed", e->line, e->col);
                        *ok = 0;
                    }
                } else if (callee.type == CS_T_FUNC) {
                    struct cs_func* fn = as_func(callee);
                    cs_env* callenv = env_new(fn->closure);
                    if (!callenv) { vm_set_err(vm, "out of memory", e->line, e->col); *ok = 0; }

                    if (*ok) {
                        if ((size_t)argc != fn->param_count) {
                            vm_set_err(vm, "wrong argument count", e->line, e->col);
                            *ok = 0;
                        } else {
                            for (size_t i = 0; i < fn->param_count; i++) {
                                env_set_here(callenv, fn->params[i], argv[i]);
                            }
                            exec_result r = exec_block(vm, callenv, fn->body);
                            if (!r.ok) *ok = 0;
                            if (r.did_return) out = cs_value_copy(r.ret);
                            cs_value_release(r.ret);
                        }
                    }
                    env_free(callenv);
                } else {
                    vm_set_err(vm, "attempted to call non-function", e->line, e->col);
                    *ok = 0;
                }
            }

            if (argv) {
                for (int i = 0; i < argc; i++) cs_value_release(argv[i]);
                free(argv);
            }
            cs_value_release(callee);
            return out;
        }

        default:
            vm_set_err(vm, "invalid expression node", e->line, e->col);
            *ok = 0;
            return cs_nil();
    }
}

static exec_result exec_block(cs_vm* vm, cs_env* env, ast* b) {
    exec_result r;
    r.did_return = 0;
    r.ret = cs_nil();
    r.ok = 1;

    if (!b || b->type != N_BLOCK) return r;

    for (size_t i = 0; i < b->as.block.count; i++) {
        r = exec_stmt(vm, env, b->as.block.items[i]);
        if (!r.ok || r.did_return) return r;
    }
    return r;
}

static exec_result exec_stmt(cs_vm* vm, cs_env* env, ast* s) {
    exec_result r; r.did_return=0; r.ret=cs_nil(); r.ok=1;
    if (!s) return r;

    switch (s->type) {
        case N_BLOCK: {
            cs_env* inner = env_new(env);
            if (!inner) { vm_set_err(vm, "out of memory", s->line, s->col); r.ok = 0; return r; }
            r = exec_block(vm, inner, s);
            env_free(inner);
            return r;
        }

        case N_LET: {
            cs_value v = cs_nil();
            if (s->as.let_stmt.init) {
                int ok = 1;
                v = eval_expr(vm, env, s->as.let_stmt.init, &ok);
                if (!ok) { r.ok = 0; return r; }
            }
            env_set_here(env, s->as.let_stmt.name, v);
            cs_value_release(v);
            return r;
        }

        case N_ASSIGN: {
            int ok = 1;
            cs_value v = eval_expr(vm, env, s->as.assign_stmt.value, &ok);
            if (!ok) { r.ok = 0; return r; }
            env_set(env, s->as.assign_stmt.name, v);
            cs_value_release(v);
            return r;
        }

        case N_FNDEF: {
            struct cs_func* f = (struct cs_func*)calloc(1, sizeof(struct cs_func));
            if (!f) { vm_set_err(vm, "out of memory", s->line, s->col); r.ok = 0; return r; }
            f->ref = 1;
            f->params = s->as.fndef.params;
            f->param_count = s->as.fndef.param_count;
            f->body = s->as.fndef.body;
            f->closure = env;

            cs_value fv; fv.type = CS_T_FUNC; fv.as.p = f;
            env_set_here(env, s->as.fndef.name, fv);
            cs_value_release(fv);
            return r;
        }

        case N_IF: {
            int ok = 1;
            cs_value c = eval_expr(vm, env, s->as.if_stmt.cond, &ok);
            if (!ok) { r.ok = 0; return r; }
            int t = is_truthy(c);
            cs_value_release(c);

            if (t) return exec_stmt(vm, env, s->as.if_stmt.then_b);
            if (s->as.if_stmt.else_b) return exec_stmt(vm, env, s->as.if_stmt.else_b);
            return r;
        }

        case N_WHILE: {
            for (;;) {
                int ok = 1;
                cs_value c = eval_expr(vm, env, s->as.while_stmt.cond, &ok);
                if (!ok) { r.ok = 0; return r; }
                int t = is_truthy(c);
                cs_value_release(c);
                if (!t) break;

                r = exec_stmt(vm, env, s->as.while_stmt.body);
                if (!r.ok || r.did_return) return r;
            }
            return r;
        }

        case N_RETURN: {
            r.did_return = 1;
            if (s->as.ret_stmt.value) {
                int ok = 1;
                r.ret = eval_expr(vm, env, s->as.ret_stmt.value, &ok);
                if (!ok) { r.ok = 0; r.did_return = 0; return r; }
            } else {
                r.ret = cs_nil();
            }
            return r;
        }

        case N_EXPR_STMT: {
            int ok = 1;
            cs_value v = eval_expr(vm, env, s->as.expr_stmt.expr, &ok);
            if (!ok) { r.ok = 0; return r; }
            cs_value_release(v);
            return r;
        }

        default:
            vm_set_err(vm, "invalid statement node", s->line, s->col);
            r.ok = 0;
            return r;
    }
}

// ---------- public API ----------
cs_vm* cs_vm_new(void) {
    cs_vm* vm = (cs_vm*)calloc(1, sizeof(cs_vm));
    if (!vm) return NULL;
    vm->globals = env_new(NULL);
    vm->last_error = NULL;
    return vm;
}

void cs_vm_free(cs_vm* vm) {
    if (!vm) return;
    env_free(vm->globals);
    free(vm->last_error);
    free(vm);
}

void cs_register_native(cs_vm* vm, const char* name, cs_native_fn fn, void* userdata) {
    if (!vm || !name || !fn) return;
    cs_native* nf = (cs_native*)calloc(1, sizeof(cs_native));
    if (!nf) return;
    nf->ref = 1;
    nf->fn = fn;
    nf->userdata = userdata;
    cs_value v; v.type = CS_T_NATIVE; v.as.p = nf;
    env_set_here(vm->globals, name, v);
    cs_value_release(v);
}

static int run_ast(cs_vm* vm, ast* prog) {
    exec_result r = exec_block(vm, vm->globals, prog);
    cs_value_release(r.ret);
    return r.ok ? 0 : -1;
}

int cs_vm_run_string(cs_vm* vm, const char* code, const char* virtual_name) {
    (void)virtual_name;
    if (!vm || !code) return -1;

    // Clear stale errors before running new code
    free(vm->last_error);
    vm->last_error = NULL;

    parser P;
    parser_init(&P, code);
    ast* prog = parse_program(&P);

    if (P.error) {
        free(vm->last_error);
        vm->last_error = cs_strdup2(P.error);
        parse_free_error(&P);
        return -1;
    }
    int rc = run_ast(vm, prog);
    // NOTE: v0 does not free AST; treat VM as owning program lifetime (fine for plugins)
    return rc;
}

static char* read_file_all(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char* buf = (char*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t r = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[r] = 0;
    return buf;
}

int cs_vm_run_file(cs_vm* vm, const char* path) {
    if (!vm || !path) return -1;
    // Clear stale errors before loading/exec a file
    free(vm->last_error);
    vm->last_error = NULL;
    char* code = read_file_all(path);
    if (!code) {
        free(vm->last_error);
        vm->last_error = cs_strdup2("could not read file");
        return -1;
    }
    int rc = cs_vm_run_string(vm, code, path);
    free(code);
    return rc;
}

int cs_call(cs_vm* vm, const char* func_name, int argc, const cs_value* argv, cs_value* out) {
    if (out) *out = cs_nil();
    if (!vm || !func_name) return -1;

    cs_value f;
    if (!env_get(vm->globals, func_name, &f)) return -1;

    int ok = 1;
    cs_value result = cs_nil();

        if (f.type == CS_T_NATIVE) {
        cs_native* nf = as_native(f);
        if (nf->fn(vm, nf->userdata, argc, argv, &result) != 0) {
            if (!vm->last_error) cs_error(vm, "native call failed");
            ok = 0;
        }
    } else if (f.type == CS_T_FUNC) {
        struct cs_func* fn = as_func(f);
        if ((size_t)argc != fn->param_count) ok = 0;
        else {
            cs_env* callenv = env_new(fn->closure);
            if (!callenv) ok = 0;
            else {
                for (size_t i = 0; i < fn->param_count; i++) env_set_here(callenv, fn->params[i], argv[i]);
                exec_result r = exec_block(vm, callenv, fn->body);
                if (!r.ok) ok = 0;
                if (r.did_return) result = cs_value_copy(r.ret);
                cs_value_release(r.ret);
                env_free(callenv);
            }
        }
    } else {
        ok = 0;
    }

    cs_value_release(f);
    if (!ok) return -1;
    if (out) *out = result;
    else cs_value_release(result);
    return 0;
}
