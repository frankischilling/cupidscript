#include "cs_vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#if !defined(_WIN32)
#include <sys/time.h>
#endif

static char* cs_strdup2_local(const char* s) {
    size_t n = strlen(s ? s : "");
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static int is_abs_path(const char* p) {
    if (!p || !*p) return 0;
    if (p[0] == '/' || p[0] == '\\') return 1;
    if (isalpha((unsigned char)p[0]) && p[1] == ':') return 1; // Windows drive
    return 0;
}

static char* path_join_alloc(const char* a, const char* b) {
    if (!a || !*a) return cs_strdup2_local(b ? b : "");
    if (!b || !*b) return cs_strdup2_local(a);

    size_t na = strlen(a), nb = strlen(b);
    int need_sep = 1;
    if (na > 0) {
        char last = a[na - 1];
        if (last == '/' || last == '\\') need_sep = 0;
    }
    if (b[0] == '/' || b[0] == '\\') need_sep = 0;

    char* out = (char*)malloc(na + (need_sep ? 1 : 0) + nb + 1);
    if (!out) return NULL;
    memcpy(out, a, na);
    size_t w = na;
    if (need_sep) out[w++] = '/';
    memcpy(out + w, b, nb + 1);
    return out;
}

static const char* vm_current_dir(const cs_vm* vm) {
    if (!vm || vm->dir_count == 0) return ".";
    return vm->dir_stack[vm->dir_count - 1];
}

static char* resolve_path_alloc(cs_vm* vm, const char* p) {
    if (is_abs_path(p)) return cs_strdup2_local(p);
    return path_join_alloc(vm_current_dir(vm), p);
}

static const char* value_repr(cs_value v, char* buf, size_t buf_sz) {
    if (!buf || buf_sz == 0) return "";
    buf[0] = 0;
    switch (v.type) {
        case CS_T_NIL:  return "nil";
        case CS_T_BOOL: return v.as.b ? "true" : "false";
        case CS_T_INT:
            snprintf(buf, buf_sz, "%lld", (long long)v.as.i);
            return buf;
        case CS_T_FLOAT:
            snprintf(buf, buf_sz, "%g", v.as.f);
            return buf;
        case CS_T_STR:
            return ((cs_string*)v.as.p)->data;
        case CS_T_LIST: {
            cs_list_obj* l = (cs_list_obj*)v.as.p;
            snprintf(buf, buf_sz, "<list len=%lld>", (long long)(l ? l->len : 0));
            return buf;
        }
        case CS_T_MAP: {
            cs_map_obj* m = (cs_map_obj*)v.as.p;
            snprintf(buf, buf_sz, "<map len=%lld>", (long long)(m ? m->len : 0));
            return buf;
        }
        case CS_T_STRBUF: {
            cs_strbuf_obj* b = (cs_strbuf_obj*)v.as.p;
            snprintf(buf, buf_sz, "<strbuf len=%lld>", (long long)(b ? b->len : 0));
            return buf;
        }
        case CS_T_FUNC:   return "<function>";
        case CS_T_NATIVE: return "<native>";
        default:          return "<obj>";
    }
}

static int nf_print(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    for (int i = 0; i < argc; i++) {
        char tmp[64];
        switch (argv[i].type) {
            case CS_T_NIL:  fputs("nil", stdout); break;
            case CS_T_BOOL: fputs(argv[i].as.b ? "true" : "false", stdout); break;
            case CS_T_INT:  fprintf(stdout, "%lld", (long long)argv[i].as.i); break;
            case CS_T_FLOAT: fprintf(stdout, "%g", argv[i].as.f); break;
            case CS_T_STR:  fputs(((cs_string*)argv[i].as.p)->data, stdout); break;
            default:        fputs(value_repr(argv[i], tmp, sizeof(tmp)), stdout); break;
        }
        if (i + 1 < argc) fputc(' ', stdout);
    }
    fputc('\n', stdout);
    if (out) *out = cs_nil();
    return 0;
}

static int nf_typeof(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (argc != 1) { if (out) *out = cs_nil(); return 0; }
    const char* tn = cs_type_name(argv[0].type);
    *out = cs_str(vm, tn);
    return 0;
}

static int nf_getenv(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (argc != 1 || argv[0].type != CS_T_STR) { *out = cs_nil(); return 0; }
    const char* key = ((cs_string*)argv[0].as.p)->data;
    const char* v = getenv(key);
    if (!v) { *out = cs_nil(); return 0; }
    *out = cs_str(vm, v);
    return 0;
}

static int truthy_local(cs_value v) {
    if (v.type == CS_T_NIL) return 0;
    if (v.type == CS_T_BOOL) return v.as.b != 0;
    return 1;
}

static int nf_assert(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (out) *out = cs_nil();
    if (argc < 1) return 0;
    if (!truthy_local(argv[0])) {
        if (argc >= 2 && argv[1].type == CS_T_STR) {
            cs_error(vm, cs_to_cstr(argv[1]));
        } else {
            cs_error(vm, "assertion failed");
        }
        return 1;
    }
    return 0;
}

static int nf_load(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (out) *out = cs_nil();
    if (argc != 1 || argv[0].type != CS_T_STR) return 0;

    char* path = resolve_path_alloc(vm, cs_to_cstr(argv[0]));
    if (!path) { cs_error(vm, "out of memory"); return 1; }

    int rc = cs_vm_run_file(vm, path);
    free(path);
    if (rc != 0) return 1;
    if (out) *out = cs_bool(1);
    return 0;
}

static int nf_require(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (out) *out = cs_nil();
    if (argc != 1 || argv[0].type != CS_T_STR) return 0;

    char* path = resolve_path_alloc(vm, cs_to_cstr(argv[0]));
    if (!path) { cs_error(vm, "out of memory"); return 1; }

    cs_value exports = cs_nil();
    int rc = cs_vm_require_module(vm, path, &exports);
    free(path);
    if (rc != 0) return 1;
    if (out) *out = exports;
    else cs_value_release(exports);
    return 0;
}

static int nf_require_optional(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (out) *out = cs_nil();
    if (argc != 1 || argv[0].type != CS_T_STR) return 0;

    char* path = resolve_path_alloc(vm, cs_to_cstr(argv[0]));
    if (!path) {
        if (out) *out = cs_nil();
        return 0;  // No error, just return nil
    }

    // Check if file exists
    FILE* f = fopen(path, "r");
    if (!f) {
        free(path);
        if (out) *out = cs_nil();
        return 0;  // No error, just return nil
    }
    fclose(f);

    // File exists, use regular require logic
    cs_value exports = cs_nil();
    int rc = cs_vm_require_module(vm, path, &exports);
    free(path);
    if (rc != 0) {
        // Even on error, return nil instead of failing
        if (out) *out = cs_nil();
        return 0;
    }
    if (out) *out = exports;
    else cs_value_release(exports);
    return 0;
}

static int list_ensure(cs_list_obj* l, size_t need) {
    if (!l) return 0;
    if (need <= l->cap) return 1;
    size_t nc = l->cap ? l->cap : 8;
    while (nc < need) nc *= 2;
    cs_value* ni = (cs_value*)realloc(l->items, nc * sizeof(cs_value));
    if (!ni) return 0;
    for (size_t i = l->cap; i < nc; i++) ni[i] = cs_nil();
    l->items = ni;
    l->cap = nc;
    return 1;
}

static int nf_list(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    *out = cs_list(vm);
    if (!out->as.p) { cs_error(vm, "out of memory"); return 1; }
    return 0;
}

static int nf_map(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    *out = cs_map(vm);
    if (!out->as.p) { cs_error(vm, "out of memory"); return 1; }
    return 0;
}

static int nf_strbuf(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    *out = cs_strbuf(vm);
    if (!out->as.p) { cs_error(vm, "out of memory"); return 1; }
    return 0;
}

static int nf_len(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_int(0); return 0; }
    if (argv[0].type == CS_T_STR) { *out = cs_int((int64_t)((cs_string*)argv[0].as.p)->len); return 0; }
    if (argv[0].type == CS_T_LIST) { *out = cs_int((int64_t)((cs_list_obj*)argv[0].as.p)->len); return 0; }
    if (argv[0].type == CS_T_MAP) { *out = cs_int((int64_t)((cs_map_obj*)argv[0].as.p)->len); return 0; }
    if (argv[0].type == CS_T_STRBUF) { *out = cs_int((int64_t)((cs_strbuf_obj*)argv[0].as.p)->len); return 0; }
    *out = cs_int(0);
    return 0;
}

static int nf_push(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (out) *out = cs_nil();
    if (argc != 2 || argv[0].type != CS_T_LIST) return 0;
    cs_list_obj* l = (cs_list_obj*)argv[0].as.p;
    if (!list_ensure(l, l->len + 1)) { cs_error(vm, "out of memory"); return 1; }
    l->items[l->len++] = cs_value_copy(argv[1]);
    return 0;
}

static int nf_pop(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_LIST) { *out = cs_nil(); return 0; }
    cs_list_obj* l = (cs_list_obj*)argv[0].as.p;
    if (!l || l->len == 0) { *out = cs_nil(); return 0; }
    *out = l->items[l->len - 1];
    l->items[l->len - 1] = cs_nil();
    l->len--;
    return 0;
}

static int map_find(cs_map_obj* m, const char* key) {
    if (!m || !key) return -1;
    for (size_t i = 0; i < m->len; i++) {
        if (strcmp(m->entries[i].key->data, key) == 0) return (int)i;
    }
    return -1;
}

static int map_ensure(cs_map_obj* m, size_t need) {
    if (!m) return 0;
    if (need <= m->cap) return 1;
    size_t nc = m->cap ? m->cap : 8;
    while (nc < need) nc *= 2;
    cs_map_entry* ne = (cs_map_entry*)realloc(m->entries, nc * sizeof(cs_map_entry));
    if (!ne) return 0;
    for (size_t i = m->cap; i < nc; i++) { ne[i].key = NULL; ne[i].val = cs_nil(); }
    m->entries = ne;
    m->cap = nc;
    return 1;
}

static int nf_mget(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_MAP || argv[1].type != CS_T_STR) { *out = cs_nil(); return 0; }
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    const char* key = ((cs_string*)argv[1].as.p)->data;
    int idx = map_find(m, key);
    if (idx < 0) { *out = cs_nil(); return 0; }
    *out = cs_value_copy(m->entries[(size_t)idx].val);
    return 0;
}

static int nf_mset(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (out) *out = cs_nil();
    if (argc != 3 || argv[0].type != CS_T_MAP || argv[1].type != CS_T_STR) return 0;
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    cs_string* key = (cs_string*)argv[1].as.p;
    int idx = map_find(m, key->data);
    if (idx >= 0) {
        cs_value_release(m->entries[(size_t)idx].val);
        m->entries[(size_t)idx].val = cs_value_copy(argv[2]);
        return 0;
    }
    if (!map_ensure(m, m->len + 1)) { cs_error(vm, "out of memory"); return 1; }
    cs_str_incref(key);
    m->entries[m->len].key = key;
    m->entries[m->len].val = cs_value_copy(argv[2]);
    m->len++;
    return 0;
}

static int nf_mhas(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_MAP || argv[1].type != CS_T_STR) { *out = cs_bool(0); return 0; }
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    const char* key = ((cs_string*)argv[1].as.p)->data;
    *out = cs_bool(map_find(m, key) >= 0);
    return 0;
}

static int nf_mdel(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_MAP || argv[1].type != CS_T_STR) { *out = cs_bool(0); return 0; }
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    const char* key = ((cs_string*)argv[1].as.p)->data;
    int idx = map_find(m, key);
    if (idx < 0) { *out = cs_bool(0); return 0; }
    size_t u = (size_t)idx;
    cs_str_decref(m->entries[u].key);
    cs_value_release(m->entries[u].val);
    for (size_t i = u + 1; i < m->len; i++) m->entries[i - 1] = m->entries[i];
    m->len--;
    *out = cs_bool(1);
    return 0;
}

static int nf_keys(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_MAP) { *out = cs_list(vm); return 0; }
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    cs_value listv = cs_list(vm);
    if (!listv.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* l = (cs_list_obj*)listv.as.p;
    if (!list_ensure(l, m->len)) { cs_value_release(listv); cs_error(vm, "out of memory"); return 1; }
    for (size_t i = 0; i < m->len; i++) {
        cs_value sv; sv.type = CS_T_STR; sv.as.p = m->entries[i].key;
        cs_str_incref(m->entries[i].key);
        l->items[l->len++] = sv;
    }
    *out = listv;
    return 0;
}

static int nf_str_find(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_STR || argv[1].type != CS_T_STR) { *out = cs_int(-1); return 0; }
    const char* s = ((cs_string*)argv[0].as.p)->data;
    const char* sub = ((cs_string*)argv[1].as.p)->data;
    const char* p = strstr(s, sub);
    if (!p) { *out = cs_int(-1); return 0; }
    *out = cs_int((int64_t)(p - s));
    return 0;
}

static int nf_str_replace(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 3 || argv[0].type != CS_T_STR || argv[1].type != CS_T_STR || argv[2].type != CS_T_STR) { *out = cs_nil(); return 0; }
    const char* s = ((cs_string*)argv[0].as.p)->data;
    const char* old = ((cs_string*)argv[1].as.p)->data;
    const char* rep = ((cs_string*)argv[2].as.p)->data;
    if (!*old) { *out = cs_str(vm, s); return 0; }

    size_t sl = strlen(s), ol = strlen(old), rl = strlen(rep);
    size_t count = 0;
    for (const char* p = s; (p = strstr(p, old)); p += ol) count++;
    size_t nl = sl;
    if (rl >= ol) nl += count * (rl - ol);
    else nl -= count * (ol - rl);
    char* buf = (char*)malloc(nl + 1);
    if (!buf) { cs_error(vm, "out of memory"); return 1; }

    size_t w = 0;
    const char* p = s;
    while (1) {
        const char* hit = strstr(p, old);
        if (!hit) break;
        size_t npre = (size_t)(hit - p);
        memcpy(buf + w, p, npre);
        w += npre;
        memcpy(buf + w, rep, rl);
        w += rl;
        p = hit + ol;
    }
    size_t tail = strlen(p);
    memcpy(buf + w, p, tail);
    w += tail;
    buf[w] = 0;
    *out = cs_str_take(vm, buf, (uint64_t)w);
    return 0;
}

static int nf_str_split(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_STR || argv[1].type != CS_T_STR) { *out = cs_list(vm); return 0; }
    const char* s = ((cs_string*)argv[0].as.p)->data;
    const char* sep = ((cs_string*)argv[1].as.p)->data;
    if (!*sep) { // return [s]
        cs_value listv = cs_list(vm);
        if (!listv.as.p) { cs_error(vm, "out of memory"); return 1; }
        cs_list_obj* l = (cs_list_obj*)listv.as.p;
        if (!list_ensure(l, 1)) { cs_value_release(listv); cs_error(vm, "out of memory"); return 1; }
        l->items[l->len++] = cs_value_copy(argv[0]);
        *out = listv;
        return 0;
    }

    cs_value listv = cs_list(vm);
    if (!listv.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* l = (cs_list_obj*)listv.as.p;

    size_t sepl = strlen(sep);
    const char* p = s;
    while (1) {
        const char* hit = strstr(p, sep);
        if (!hit) break;
        size_t n = (size_t)(hit - p);
        char* part = (char*)malloc(n + 1);
        if (!part) { cs_value_release(listv); cs_error(vm, "out of memory"); return 1; }
        memcpy(part, p, n);
        part[n] = 0;
        cs_value sv = cs_str_take(vm, part, (uint64_t)n);
        if (!list_ensure(l, l->len + 1)) { cs_value_release(sv); cs_value_release(listv); cs_error(vm, "out of memory"); return 1; }
        l->items[l->len++] = sv;
        p = hit + sepl;
    }
    cs_value tailv = cs_str(vm, p);
    if (!list_ensure(l, l->len + 1)) { cs_value_release(tailv); cs_value_release(listv); cs_error(vm, "out of memory"); return 1; }
    l->items[l->len++] = tailv;

    *out = listv;
    return 0;
}

static int nf_path_join(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_STR || argv[1].type != CS_T_STR) { *out = cs_nil(); return 0; }
    char* joined = path_join_alloc(((cs_string*)argv[0].as.p)->data, ((cs_string*)argv[1].as.p)->data);
    if (!joined) { cs_error(vm, "out of memory"); return 1; }
    *out = cs_str_take(vm, joined, (uint64_t)-1);
    return 0;
}

static int nf_path_dirname(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_STR) { *out = cs_nil(); return 0; }
    const char* path = ((cs_string*)argv[0].as.p)->data;
    const char* last_slash = strrchr(path, '/');
    const char* last_bslash = strrchr(path, '\\');
    const char* last = last_slash;
    if (last_bslash && (!last || last_bslash > last)) last = last_bslash;
    if (!last) { *out = cs_str(vm, "."); return 0; }
    size_t n = (size_t)(last - path);
    if (n == 0) { *out = cs_str(vm, "/"); return 0; }
    char* buf = (char*)malloc(n + 1);
    if (!buf) { cs_error(vm, "out of memory"); return 1; }
    memcpy(buf, path, n);
    buf[n] = 0;
    *out = cs_str_take(vm, buf, (uint64_t)n);
    return 0;
}

static int nf_path_basename(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_STR) { *out = cs_nil(); return 0; }
    const char* path = ((cs_string*)argv[0].as.p)->data;
    const char* last_slash = strrchr(path, '/');
    const char* last_bslash = strrchr(path, '\\');
    const char* last = last_slash;
    if (last_bslash && (!last || last_bslash > last)) last = last_bslash;
    const char* base = last ? last + 1 : path;
    *out = cs_str(vm, base);
    return 0;
}

static int nf_path_ext(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_STR) { *out = cs_nil(); return 0; }
    const char* path = ((cs_string*)argv[0].as.p)->data;
    const char* base = path;
    const char* last_slash = strrchr(path, '/');
    const char* last_bslash = strrchr(path, '\\');
    const char* last = last_slash;
    if (last_bslash && (!last || last_bslash > last)) last = last_bslash;
    if (last) base = last + 1;
    const char* dot = strrchr(base, '.');
    if (!dot || dot == base) { *out = cs_str(vm, ""); return 0; }
    *out = cs_str(vm, dot + 1);
    return 0;
}

static int sb_append(char** buf, size_t* len, size_t* cap, const char* s, size_t n) {
    if (!buf || !len || !cap) return 0;
    if (*len + n + 1 > *cap) {
        size_t nc = (*cap == 0) ? 64 : *cap;
        while (*len + n + 1 > nc) nc *= 2;
        char* nb = (char*)realloc(*buf, nc);
        if (!nb) return 0;
        *buf = nb;
        *cap = nc;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = 0;
    return 1;
}

static int nf_fmt(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_STR) { *out = cs_nil(); return 0; }

    const char* fmt = ((cs_string*)argv[0].as.p)->data;
    char* buf = NULL;
    size_t len = 0, cap = 0;
    int ai = 1;

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            if (!sb_append(&buf, &len, &cap, &fmt[i], 1)) { free(buf); cs_error(vm, "out of memory"); return 1; }
            continue;
        }
        char spec = fmt[++i];
        if (!spec) break;
        if (spec == '%') {
            if (!sb_append(&buf, &len, &cap, "%", 1)) { free(buf); cs_error(vm, "out of memory"); return 1; }
            continue;
        }
        if (ai >= argc) { free(buf); cs_error(vm, "fmt: not enough arguments"); return 1; }

        char tmp[128];
        tmp[0] = 0;
        const cs_value v = argv[ai++];
        const char* s = NULL;
        size_t sn = 0;

        if (spec == 'd') {
            if (v.type != CS_T_INT) { free(buf); cs_error(vm, "fmt: %d expects int"); return 1; }
            snprintf(tmp, sizeof(tmp), "%lld", (long long)v.as.i);
            s = tmp;
        } else if (spec == 'b') {
            if (v.type != CS_T_BOOL) { free(buf); cs_error(vm, "fmt: %b expects bool"); return 1; }
            s = v.as.b ? "true" : "false";
        } else if (spec == 's') {
            if (v.type != CS_T_STR) { free(buf); cs_error(vm, "fmt: %s expects string"); return 1; }
            s = ((cs_string*)v.as.p)->data;
        } else if (spec == 'v') {
            s = value_repr(v, tmp, sizeof(tmp));
        } else {
            free(buf);
            cs_error(vm, "fmt: unknown format specifier");
            return 1;
        }

        sn = strlen(s);
        if (!sb_append(&buf, &len, &cap, s, sn)) { free(buf); cs_error(vm, "out of memory"); return 1; }
    }

    if (!buf) buf = cs_strdup2_local("");
    if (!buf) { cs_error(vm, "out of memory"); return 1; }
    *out = cs_str_take(vm, buf, (uint64_t)len);
    return 0;
}

static int nf_now_ms(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
#if !defined(_WIN32)
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) { *out = cs_int(0); return 0; }
    int64_t ms = (int64_t)tv.tv_sec * 1000 + (int64_t)(tv.tv_usec / 1000);
    *out = cs_int(ms);
    return 0;
#else
    *out = cs_int((int64_t)(clock() * 1000 / (int64_t)CLOCKS_PER_SEC));
    return 0;
#endif
}

static int nf_sleep(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (out) *out = cs_nil();
    if (argc != 1 || argv[0].type != CS_T_INT) return 0;
    int64_t ms = argv[0].as.i;
    if (ms <= 0) return 0;
#if !defined(_WIN32)
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000L);
    (void)nanosleep(&ts, NULL);
    return 0;
#else
    (void)vm;
    // No portable C99 sleep for Windows here; host should provide its own.
    cs_error(vm, "sleep not supported on this platform in stdlib");
    return 1;
#endif
}

static int nf_values(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_MAP) { *out = cs_list(vm); return 0; }
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    cs_value listv = cs_list(vm);
    if (!listv.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* l = (cs_list_obj*)listv.as.p;
    if (!list_ensure(l, m->len)) { cs_value_release(listv); cs_error(vm, "out of memory"); return 1; }
    for (size_t i = 0; i < m->len; i++) l->items[l->len++] = cs_value_copy(m->entries[i].val);
    *out = listv;
    return 0;
}

static int nf_items(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1 || argv[0].type != CS_T_MAP) { *out = cs_list(vm); return 0; }
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    cs_value outer = cs_list(vm);
    if (!outer.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* ol = (cs_list_obj*)outer.as.p;
    if (!list_ensure(ol, m->len)) { cs_value_release(outer); cs_error(vm, "out of memory"); return 1; }

    for (size_t i = 0; i < m->len; i++) {
        cs_value pair = cs_list(vm);
        if (!pair.as.p) { cs_value_release(outer); cs_error(vm, "out of memory"); return 1; }
        cs_list_obj* pl = (cs_list_obj*)pair.as.p;
        if (!list_ensure(pl, 2)) { cs_value_release(pair); cs_value_release(outer); cs_error(vm, "out of memory"); return 1; }

        cs_value kv; kv.type = CS_T_STR; kv.as.p = m->entries[i].key; cs_str_incref(m->entries[i].key);
        pl->items[pl->len++] = kv;
        pl->items[pl->len++] = cs_value_copy(m->entries[i].val);

        ol->items[ol->len++] = pair;
    }

    *out = outer;
    return 0;
}

static int nf_insert(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (out) *out = cs_nil();
    if (argc != 3 || argv[0].type != CS_T_LIST || argv[1].type != CS_T_INT) return 0;
    cs_list_obj* l = (cs_list_obj*)argv[0].as.p;
    int64_t idx = argv[1].as.i;
    if (idx < 0) idx = 0;
    if ((size_t)idx > l->len) idx = (int64_t)l->len;
    if (!list_ensure(l, l->len + 1)) { cs_error(vm, "out of memory"); return 1; }
    for (size_t i = l->len; i > (size_t)idx; i--) l->items[i] = l->items[i - 1];
    l->items[(size_t)idx] = cs_value_copy(argv[2]);
    l->len++;
    return 0;
}

static int nf_remove(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_LIST || argv[1].type != CS_T_INT) { *out = cs_nil(); return 0; }
    cs_list_obj* l = (cs_list_obj*)argv[0].as.p;
    int64_t idx = argv[1].as.i;
    if (idx < 0 || (size_t)idx >= l->len) { *out = cs_nil(); return 0; }
    size_t u = (size_t)idx;
    cs_value removed = l->items[u];
    for (size_t i = u + 1; i < l->len; i++) l->items[i - 1] = l->items[i];
    l->len--;
    l->items[l->len] = cs_nil();
    *out = removed;
    return 0;
}

static int nf_slice(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 3 || argv[0].type != CS_T_LIST || argv[1].type != CS_T_INT || argv[2].type != CS_T_INT) { *out = cs_list(vm); return 0; }
    cs_list_obj* l = (cs_list_obj*)argv[0].as.p;
    int64_t start = argv[1].as.i;
    int64_t end = argv[2].as.i;
    if (start < 0) start = 0;
    if (end < start) end = start;
    if ((size_t)end > l->len) end = (int64_t)l->len;
    cs_value outl = cs_list(vm);
    if (!outl.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* ol = (cs_list_obj*)outl.as.p;
    if (!list_ensure(ol, (size_t)(end - start))) { cs_value_release(outl); cs_error(vm, "out of memory"); return 1; }
    for (int64_t i = start; i < end; i++) ol->items[ol->len++] = cs_value_copy(l->items[(size_t)i]);
    *out = outl;
    return 0;
}

static int nf_substr(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 3 || argv[0].type != CS_T_STR || argv[1].type != CS_T_INT || argv[2].type != CS_T_INT) { *out = cs_nil(); return 0; }
    cs_string* s = (cs_string*)argv[0].as.p;
    int64_t start = argv[1].as.i;
    int64_t n = argv[2].as.i;
    if (start < 0) start = 0;
    if (n < 0) n = 0;
    if ((size_t)start > s->len) start = (int64_t)s->len;
    size_t maxn = s->len - (size_t)start;
    size_t take = (size_t)n;
    if (take > maxn) take = maxn;
    char* buf = (char*)malloc(take + 1);
    if (!buf) { cs_error(vm, "out of memory"); return 1; }
    memcpy(buf, s->data + start, take);
    buf[take] = 0;
    *out = cs_str_take(vm, buf, (uint64_t)take);
    return 0;
}

static int nf_to_int(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_nil(); return 0; }
    if (argv[0].type == CS_T_INT) { *out = cs_int(argv[0].as.i); return 0; }
    if (argv[0].type == CS_T_BOOL) { *out = cs_int(argv[0].as.b ? 1 : 0); return 0; }
    if (argv[0].type != CS_T_STR) { *out = cs_nil(); return 0; }
    const char* p = ((cs_string*)argv[0].as.p)->data;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    int sign = 1;
    if (*p == '-') { sign = -1; p++; }
    else if (*p == '+') { p++; }
    int64_t v = 0;
    int any = 0;
    while (*p >= '0' && *p <= '9') { any = 1; v = v * 10 + (*p - '0'); p++; }
    if (!any) { *out = cs_nil(); return 0; }
    *out = cs_int(v * sign);
    return 0;
}

static int nf_to_str(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_str(vm, ""); return 0; }
    char tmp[128];
    const char* s = value_repr(argv[0], tmp, sizeof(tmp));
    *out = cs_str(vm, s);
    return 0;
}

static int nf_join(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc != 2 || argv[0].type != CS_T_LIST || argv[1].type != CS_T_STR) { *out = cs_nil(); return 0; }
    cs_list_obj* l = (cs_list_obj*)argv[0].as.p;
    const char* sep = ((cs_string*)argv[1].as.p)->data;
    size_t sepn = strlen(sep);

    char* buf = NULL;
    size_t len = 0, cap = 0;
    for (size_t i = 0; i < l->len; i++) {
        if (i > 0) {
            if (!sb_append(&buf, &len, &cap, sep, sepn)) { free(buf); cs_error(vm, "out of memory"); return 1; }
        }
        char tmp[128];
        const char* part = value_repr(l->items[i], tmp, sizeof(tmp));
        size_t pn = strlen(part);
        if (!sb_append(&buf, &len, &cap, part, pn)) { free(buf); cs_error(vm, "out of memory"); return 1; }
    }
    if (!buf) buf = cs_strdup2_local("");
    if (!buf) { cs_error(vm, "out of memory"); return 1; }
    *out = cs_str_take(vm, buf, (uint64_t)len);
    return 0;
}

static int nf_gc(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    *out = cs_int((int64_t)cs_vm_collect_cycles(vm));
    return 0;
}

static int nf_gc_stats(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    
    cs_value stats_map = cs_map(vm);
    if (stats_map.type != CS_T_MAP) {
        *out = cs_nil();
        return 0;
    }
    
    // Get internal VM GC stats
    extern size_t vm_get_tracked_count(cs_vm* vm);
    extern size_t vm_get_gc_collections(cs_vm* vm);
    extern size_t vm_get_gc_objects_collected(cs_vm* vm);
    extern size_t vm_get_gc_allocations(cs_vm* vm);
    
    cs_map_set(stats_map, "tracked", cs_int((int64_t)vm_get_tracked_count(vm)));
    cs_map_set(stats_map, "collections", cs_int((int64_t)vm_get_gc_collections(vm)));
    cs_map_set(stats_map, "collected", cs_int((int64_t)vm_get_gc_objects_collected(vm)));
    cs_map_set(stats_map, "allocations", cs_int((int64_t)vm_get_gc_allocations(vm)));
    
    *out = stats_map;
    return 0;
}

static int nf_gc_config(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    
    // gc_config() - get current config
    if (argc == 0) {
        cs_value config_map = cs_map(vm);
        if (config_map.type != CS_T_MAP) {
            *out = cs_nil();
            return 0;
        }
        
        extern size_t vm_get_gc_threshold(cs_vm* vm);
        extern size_t vm_get_gc_alloc_trigger(cs_vm* vm);
        
        cs_map_set(config_map, "threshold", cs_int((int64_t)vm_get_gc_threshold(vm)));
        cs_map_set(config_map, "alloc_trigger", cs_int((int64_t)vm_get_gc_alloc_trigger(vm)));
        
        *out = config_map;
        return 0;
    }
    
    // gc_config(map) - set config from map
    if (argc == 1 && argv[0].type == CS_T_MAP) {
        cs_value threshold_val = cs_map_get(argv[0], "threshold");
        if (threshold_val.type == CS_T_INT) {
            cs_vm_set_gc_threshold(vm, (size_t)threshold_val.as.i);
        }
        cs_value_release(threshold_val);
        
        cs_value trigger_val = cs_map_get(argv[0], "alloc_trigger");
        if (trigger_val.type == CS_T_INT) {
            cs_vm_set_gc_alloc_trigger(vm, (size_t)trigger_val.as.i);
        }
        cs_value_release(trigger_val);
        
        *out = cs_bool(1);
        return 0;
    }
    
    // gc_config(threshold, alloc_trigger) - set both
    if (argc == 2) {
        if (argv[0].type == CS_T_INT) {
            cs_vm_set_gc_threshold(vm, (size_t)argv[0].as.i);
        }
        if (argv[1].type == CS_T_INT) {
            cs_vm_set_gc_alloc_trigger(vm, (size_t)argv[1].as.i);
        }
        *out = cs_bool(1);
        return 0;
    }
    
    *out = cs_nil();
    return 0;
}

static int nf_set_timeout(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    
    if (argc < 1 || argv[0].type != CS_T_INT) {
        cs_error(vm, "set_timeout() requires an integer (milliseconds)");
        return 1;
    }
    
    cs_vm_set_timeout(vm, (uint64_t)argv[0].as.i);
    *out = cs_bool(1);
    return 0;
}

static int nf_set_instruction_limit(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    
    if (argc < 1 || argv[0].type != CS_T_INT) {
        cs_error(vm, "set_instruction_limit() requires an integer");
        return 1;
    }
    
    cs_vm_set_instruction_limit(vm, (uint64_t)argv[0].as.i);
    *out = cs_bool(1);
    return 0;
}

static int nf_get_timeout(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    
    extern uint64_t vm_get_timeout(cs_vm* vm);
    *out = cs_int((int64_t)vm_get_timeout(vm));
    return 0;
}

static int nf_get_instruction_limit(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    
    extern uint64_t vm_get_instruction_limit(cs_vm* vm);
    *out = cs_int((int64_t)vm_get_instruction_limit(vm));
    return 0;
}

static int nf_get_instruction_count(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud; (void)argc; (void)argv;
    if (!out) return 0;
    
    *out = cs_int((int64_t)cs_vm_get_instruction_count(vm));
    return 0;
}

static int nf_range(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    
    int64_t start = 0, end = 0, step = 1;
    
    // range(end) or range(start, end) or range(start, end, step)
    if (argc == 1) {
        if (argv[0].type != CS_T_INT) { *out = cs_nil(); return 0; }
        end = argv[0].as.i;
    } else if (argc == 2) {
        if (argv[0].type != CS_T_INT || argv[1].type != CS_T_INT) { *out = cs_nil(); return 0; }
        start = argv[0].as.i;
        end = argv[1].as.i;
    } else if (argc >= 3) {
        if (argv[0].type != CS_T_INT || argv[1].type != CS_T_INT || argv[2].type != CS_T_INT) { *out = cs_nil(); return 0; }
        start = argv[0].as.i;
        end = argv[1].as.i;
        step = argv[2].as.i;
    } else {
        *out = cs_nil();
        return 0;
    }
    
    if (step == 0) {
        cs_error(vm, "range() step cannot be zero");
        return 1;
    }
    
    cs_value lv = cs_list(vm);
    if (!lv.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* l = (cs_list_obj*)lv.as.p;
    
    if (step > 0) {
        for (int64_t i = start; i < end; i += step) {
            cs_value v = cs_int(i);
            if (!list_ensure(l, l->len + 1)) {
                cs_value_release(lv);
                cs_error(vm, "out of memory");
                return 1;
            }
            l->items[l->len++] = v;
        }
    } else {
        for (int64_t i = start; i > end; i += step) {
            cs_value v = cs_int(i);
            if (!list_ensure(l, l->len + 1)) {
                cs_value_release(lv);
                cs_error(vm, "out of memory");
                return 1;
            }
            l->items[l->len++] = v;
        }
    }
    
    *out = lv;
    return 0;
}

// Type predicates
static int nf_is_nil(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_NIL);
    return 0;
}

static int nf_is_bool(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_BOOL);
    return 0;
}

static int nf_is_int(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_INT);
    return 0;
}

static int nf_is_float(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_FLOAT);
    return 0;
}

static int nf_is_string(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_STR);
    return 0;
}

static int nf_is_list(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_LIST);
    return 0;
}

static int nf_is_map(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && argv[0].type == CS_T_MAP);
    return 0;
}

static int nf_is_function(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    *out = cs_bool(argc > 0 && (argv[0].type == CS_T_FUNC || argv[0].type == CS_T_NATIVE));
    return 0;
}

// Math functions
static double to_number(cs_value v) {
    if (v.type == CS_T_INT) return (double)v.as.i;
    if (v.type == CS_T_FLOAT) return v.as.f;
    return 0.0;
}

static int nf_abs(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_nil(); return 0; }
    
    if (argv[0].type == CS_T_INT) {
        int64_t v = argv[0].as.i;
        *out = cs_int(v < 0 ? -v : v);
    } else if (argv[0].type == CS_T_FLOAT) {
        *out = cs_float(fabs(argv[0].as.f));
    } else {
        *out = cs_nil();
    }
    return 0;
}

static int nf_min(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc < 2) { *out = argc == 1 ? argv[0] : cs_nil(); return 0; }
    
    cs_value min_val = argv[0];
    double min_num = to_number(argv[0]);
    for (int i = 1; i < argc; i++) {
        double v = to_number(argv[i]);
        if (v < min_num) {
            min_num = v;
            min_val = argv[i];
        }
    }
    *out = cs_value_copy(min_val);
    return 0;
}

static int nf_max(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc < 2) { *out = argc == 1 ? argv[0] : cs_nil(); return 0; }
    
    cs_value max_val = argv[0];
    double max_num = to_number(argv[0]);
    for (int i = 1; i < argc; i++) {
        double v = to_number(argv[i]);
        if (v > max_num) {
            max_num = v;
            max_val = argv[i];
        }
    }
    *out = cs_value_copy(max_val);
    return 0;
}

static int nf_floor(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_nil(); return 0; }
    
    double v = to_number(argv[0]);
    *out = cs_int((int64_t)floor(v));
    return 0;
}

static int nf_ceil(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_nil(); return 0; }
    
    double v = to_number(argv[0]);
    *out = cs_int((int64_t)ceil(v));
    return 0;
}

static int nf_round(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_nil(); return 0; }
    
    double v = to_number(argv[0]);
    *out = cs_int((int64_t)round(v));
    return 0;
}

static int nf_sqrt(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 1) { *out = cs_nil(); return 0; }
    
    double v = to_number(argv[0]);
    *out = cs_float(sqrt(v));
    return 0;
}

static int nf_pow(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc != 2) { *out = cs_nil(); return 0; }
    
    double base = to_number(argv[0]);
    double exp = to_number(argv[1]);
    *out = cs_float(pow(base, exp));
    return 0;
}

// String ergonomics functions
static int cs_value_equals(const cs_value* a, const cs_value* b);  // forward decl

static int cs_value_equals(const cs_value* a, const cs_value* b) {
    if (a->type != b->type) return 0;
    
    switch (a->type) {
        case CS_T_NIL:
            return 1;
        case CS_T_BOOL:
            return a->as.b == b->as.b;
        case CS_T_INT:
            return a->as.i == b->as.i;
        case CS_T_FLOAT:
            return a->as.f == b->as.f;
        case CS_T_STR:
            return strcmp(cs_to_cstr(*a), cs_to_cstr(*b)) == 0;
        case CS_T_LIST: {
            cs_list_obj* la = (cs_list_obj*)a->as.p;
            cs_list_obj* lb = (cs_list_obj*)b->as.p;
            if (la->len != lb->len) return 0;
            for (size_t i = 0; i < la->len; i++) {
                if (!cs_value_equals(&la->items[i], &lb->items[i])) {
                    return 0;
                }
            }
            return 1;
        }
        case CS_T_MAP: {
            cs_map_obj* ma = (cs_map_obj*)a->as.p;
            cs_map_obj* mb = (cs_map_obj*)b->as.p;
            if (ma->len != mb->len) return 0;
            for (size_t i = 0; i < ma->len; i++) {
                int idx = map_find(mb, ma->entries[i].key->data);
                if (idx < 0) return 0;
                if (!cs_value_equals(&ma->entries[i].val, &mb->entries[(size_t)idx].val)) {
                    return 0;
                }
            }
            return 1;
        }
        default:
            return a->as.p == b->as.p;
    }
}

static int nf_str_trim(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_STR) { cs_error(vm, "str_trim() requires a string argument"); return 1; }
    
    const char* s = cs_to_cstr(argv[0]);
    while (*s && isspace((unsigned char)*s)) s++;
    
    const char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) end--;
    
    size_t len = (size_t)(end - s);
    char* trimmed = (char*)malloc(len + 1);
    if (!trimmed) { cs_error(vm, "out of memory"); return 1; }
    memcpy(trimmed, s, len);
    trimmed[len] = '\0';
    
    *out = cs_str(vm, trimmed);
    free(trimmed);
    return 0;
}

static int nf_str_ltrim(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_STR) { cs_error(vm, "str_ltrim() requires a string argument"); return 1; }
    
    const char* s = cs_to_cstr(argv[0]);
    while (*s && isspace((unsigned char)*s)) s++;
    
    *out = cs_str(vm, s);
    return 0;
}

static int nf_str_rtrim(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_STR) { cs_error(vm, "str_rtrim() requires a string argument"); return 1; }
    
    const char* s = cs_to_cstr(argv[0]);
    const char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) end--;
    
    size_t len = (size_t)(end - s);
    char* trimmed = (char*)malloc(len + 1);
    if (!trimmed) { cs_error(vm, "out of memory"); return 1; }
    memcpy(trimmed, s, len);
    trimmed[len] = '\0';
    
    *out = cs_str(vm, trimmed);
    free(trimmed);
    return 0;
}

static int nf_str_lower(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_STR) { cs_error(vm, "str_lower() requires a string argument"); return 1; }
    
    const char* s = cs_to_cstr(argv[0]);
    size_t len = strlen(s);
    char* lower = (char*)malloc(len + 1);
    if (!lower) { cs_error(vm, "out of memory"); return 1; }
    
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)s[i]);
    }
    lower[len] = '\0';
    
    *out = cs_str(vm, lower);
    free(lower);
    return 0;
}

static int nf_str_upper(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_STR) { cs_error(vm, "str_upper() requires a string argument"); return 1; }
    
    const char* s = cs_to_cstr(argv[0]);
    size_t len = strlen(s);
    char* upper = (char*)malloc(len + 1);
    if (!upper) { cs_error(vm, "out of memory"); return 1; }
    
    for (size_t i = 0; i < len; i++) {
        upper[i] = (char)toupper((unsigned char)s[i]);
    }
    upper[len] = '\0';
    
    *out = cs_str(vm, upper);
    free(upper);
    return 0;
}

static int nf_str_startswith(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 2 || argv[0].type != CS_T_STR || argv[1].type != CS_T_STR) { 
        cs_error(vm, "str_startswith() requires 2 string arguments"); 
        return 1; 
    }
    
    const char* s = cs_to_cstr(argv[0]);
    const char* prefix = cs_to_cstr(argv[1]);
    size_t prefix_len = strlen(prefix);
    
    *out = cs_bool(strncmp(s, prefix, prefix_len) == 0);
    return 0;
}

static int nf_str_endswith(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 2 || argv[0].type != CS_T_STR || argv[1].type != CS_T_STR) { 
        cs_error(vm, "str_endswith() requires 2 string arguments"); 
        return 1; 
    }
    
    const char* s = cs_to_cstr(argv[0]);
    const char* suffix = cs_to_cstr(argv[1]);
    
    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > s_len) {
        *out = cs_bool(0);
        return 0;
    }
    
    *out = cs_bool(strcmp(s + s_len - suffix_len, suffix) == 0);
    return 0;
}

static int nf_str_repeat(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 2 || argv[0].type != CS_T_STR || argv[1].type != CS_T_INT) { 
        cs_error(vm, "str_repeat() requires a string and an integer"); 
        return 1; 
    }
    
    const char* s = cs_to_cstr(argv[0]);
    int64_t count = argv[1].as.i;
    
    if (count <= 0) {
        *out = cs_str(vm, "");
        return 0;
    }
    
    size_t s_len = strlen(s);
    size_t total_len = s_len * (size_t)count;
    char* result = (char*)malloc(total_len + 1);
    if (!result) { cs_error(vm, "out of memory"); return 1; }
    
    char* p = result;
    for (int64_t i = 0; i < count; i++) {
        memcpy(p, s, s_len);
        p += s_len;
    }
    *p = '\0';
    
    *out = cs_str(vm, result);
    free(result);
    return 0;
}

// Data quality-of-life functions
static int nf_map_values(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_MAP) { cs_error(vm, "map_values() requires a map argument"); return 1; }
    
    cs_map_obj* m = (cs_map_obj*)argv[0].as.p;
    cs_value values_list = cs_list(vm);
    if (!values_list.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* l = (cs_list_obj*)values_list.as.p;
    
    if (!list_ensure(l, m->len)) { cs_value_release(values_list); cs_error(vm, "out of memory"); return 1; }
    for (size_t i = 0; i < m->len; i++) {
        l->items[l->len++] = cs_value_copy(m->entries[i].val);
    }
    
    *out = values_list;
    return 0;
}

static int nf_copy(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1) { cs_error(vm, "copy() requires 1 argument"); return 1; }
    
    cs_value src = argv[0];
    
    switch (src.type) {
        case CS_T_LIST: {
            cs_value new_list = cs_list(vm);
            if (!new_list.as.p) { cs_error(vm, "out of memory"); return 1; }
            cs_list_obj* src_list = (cs_list_obj*)src.as.p;
            cs_list_obj* dst_list = (cs_list_obj*)new_list.as.p;
            
            if (!list_ensure(dst_list, src_list->len)) {
                cs_value_release(new_list);
                cs_error(vm, "out of memory");
                return 1;
            }
            
            for (size_t i = 0; i < src_list->len; i++) {
                dst_list->items[dst_list->len++] = cs_value_copy(src_list->items[i]);
            }
            
            *out = new_list;
            return 0;
        }
        
        case CS_T_MAP: {
            cs_value new_map = cs_map(vm);
            if (!new_map.as.p) { cs_error(vm, "out of memory"); return 1; }
            cs_map_obj* src_map = (cs_map_obj*)src.as.p;
            cs_map_obj* dst_map = (cs_map_obj*)new_map.as.p;
            
            if (!map_ensure(dst_map, src_map->len)) {
                cs_value_release(new_map);
                cs_error(vm, "out of memory");
                return 1;
            }
            
            for (size_t i = 0; i < src_map->len; i++) {
                cs_str_incref(src_map->entries[i].key);
                dst_map->entries[dst_map->len].key = src_map->entries[i].key;
                dst_map->entries[dst_map->len].val = cs_value_copy(src_map->entries[i].val);
                dst_map->len++;
            }
            
            *out = new_map;
            return 0;
        }
        
        default:
            *out = cs_value_copy(src);
            return 0;
    }
}

static cs_value deepcopy_impl(cs_vm* vm, cs_value src, cs_map_obj* visited_map);

static cs_value deepcopy_impl(cs_vm* vm, cs_value src, cs_map_obj* visited_map) {
    // Check for cycles
    if (src.type == CS_T_LIST || src.type == CS_T_MAP) {
        char ptr_str[32];
        snprintf(ptr_str, sizeof(ptr_str), "%p", src.as.p);
        
        int idx = map_find(visited_map, ptr_str);
        if (idx >= 0) {
            return cs_value_copy(visited_map->entries[(size_t)idx].val);
        }
    }
    
    switch (src.type) {
        case CS_T_LIST: {
            cs_value new_list = cs_list(vm);
            cs_list_obj* src_list = (cs_list_obj*)src.as.p;
            cs_list_obj* dst_list = (cs_list_obj*)new_list.as.p;
            
            // Register in visited map
            char ptr_str[32];
            snprintf(ptr_str, sizeof(ptr_str), "%p", src.as.p);
            cs_string* key = cs_str_new(ptr_str);
            if (!map_ensure(visited_map, visited_map->len + 1)) return cs_nil();
            visited_map->entries[visited_map->len].key = key;
            visited_map->entries[visited_map->len].val = cs_value_copy(new_list);
            visited_map->len++;
            
            if (!list_ensure(dst_list, src_list->len)) return cs_nil();
            for (size_t i = 0; i < src_list->len; i++) {
                cs_value item = deepcopy_impl(vm, src_list->items[i], visited_map);
                dst_list->items[dst_list->len++] = item;
            }
            
            return new_list;
        }
        
        case CS_T_MAP: {
            cs_value new_map = cs_map(vm);
            cs_map_obj* src_map = (cs_map_obj*)src.as.p;
            cs_map_obj* dst_map = (cs_map_obj*)new_map.as.p;
            
            // Register in visited map
            char ptr_str[32];
            snprintf(ptr_str, sizeof(ptr_str), "%p", src.as.p);
            cs_string* key = cs_str_new(ptr_str);
            if (!map_ensure(visited_map, visited_map->len + 1)) return cs_nil();
            visited_map->entries[visited_map->len].key = key;
            visited_map->entries[visited_map->len].val = cs_value_copy(new_map);
            visited_map->len++;
            
            if (!map_ensure(dst_map, src_map->len)) return cs_nil();
            for (size_t i = 0; i < src_map->len; i++) {
                cs_value val = deepcopy_impl(vm, src_map->entries[i].val, visited_map);
                cs_str_incref(src_map->entries[i].key);
                dst_map->entries[dst_map->len].key = src_map->entries[i].key;
                dst_map->entries[dst_map->len].val = val;
                dst_map->len++;
            }
            
            return new_map;
        }
        
        default:
            return cs_value_copy(src);
    }
}

static int nf_deepcopy(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1) { cs_error(vm, "deepcopy() requires 1 argument"); return 1; }
    
    cs_value visited = cs_map(vm);
    if (!visited.as.p) { cs_error(vm, "out of memory"); return 1; }
    *out = deepcopy_impl(vm, argv[0], (cs_map_obj*)visited.as.p);
    cs_value_release(visited);
    
    return 0;
}

static int nf_reverse(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_LIST) { cs_error(vm, "reverse() requires a list argument"); return 1; }
    
    cs_list_obj* list = (cs_list_obj*)argv[0].as.p;
    
    for (size_t i = 0; i < list->len / 2; i++) {
        size_t j = list->len - 1 - i;
        cs_value temp = list->items[i];
        list->items[i] = list->items[j];
        list->items[j] = temp;
    }
    
    *out = cs_nil();
    return 0;
}

static int nf_reversed(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1 || argv[0].type != CS_T_LIST) { cs_error(vm, "reversed() requires a list argument"); return 1; }
    
    cs_list_obj* src_list = (cs_list_obj*)argv[0].as.p;
    cs_value rev_list = cs_list(vm);
    if (!rev_list.as.p) { cs_error(vm, "out of memory"); return 1; }
    cs_list_obj* dst_list = (cs_list_obj*)rev_list.as.p;
    
    if (!list_ensure(dst_list, src_list->len)) {
        cs_value_release(rev_list);
        cs_error(vm, "out of memory");
        return 1;
    }
    
    for (size_t i = src_list->len; i > 0; i--) {
        dst_list->items[dst_list->len++] = cs_value_copy(src_list->items[i - 1]);
    }
    
    *out = rev_list;
    return 0;
}

static int nf_contains(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 2) { cs_error(vm, "contains() requires 2 arguments"); return 1; }
    
    cs_value container = argv[0];
    cs_value item = argv[1];
    
    switch (container.type) {
        case CS_T_LIST: {
            cs_list_obj* list = (cs_list_obj*)container.as.p;
            for (size_t i = 0; i < list->len; i++) {
                if (cs_value_equals(&list->items[i], &item)) {
                    *out = cs_bool(1);
                    return 0;
                }
            }
            *out = cs_bool(0);
            return 0;
        }
        
        case CS_T_MAP: {
            if (item.type != CS_T_STR) {
                cs_error(vm, "Map keys must be strings");
                return 1;
            }
            cs_map_obj* m = (cs_map_obj*)container.as.p;
            *out = cs_bool(map_find(m, cs_to_cstr(item)) >= 0);
            return 0;
        }
        
        case CS_T_STR: {
            if (item.type != CS_T_STR) {
                *out = cs_bool(0);
                return 0;
            }
            const char* haystack = cs_to_cstr(container);
            const char* needle = cs_to_cstr(item);
            *out = cs_bool(strstr(haystack, needle) != NULL);
            return 0;
        }
        
        default:
            cs_error(vm, "contains() requires a list, map, or string");
            return 1;
    }
}

// Error object functions
static int nf_error(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)ud;
    if (!out) return 0;
    if (argc < 1) { cs_error(vm, "error() requires at least 1 argument"); return 1; }
    
    cs_value err_map = cs_map(vm);
    if (!err_map.as.p) { cs_error(vm, "out of memory"); return 1; }
    
    // Set message (required)
    const char* msg = (argv[0].type == CS_T_STR) ? cs_to_cstr(argv[0]) : "Error";
    cs_value msg_val = cs_str(vm, msg);
    if (!msg_val.as.p) { cs_value_release(err_map); cs_error(vm, "out of memory"); return 1; }
    
    cs_value args[3];
    args[0] = err_map;
    args[1] = cs_str(vm, "msg");
    args[2] = msg_val;
    nf_mset(vm, NULL, 3, args, NULL);
    cs_value_release(msg_val);
    cs_value_release(args[1]);
    
    // Set code (optional, default to "ERROR")
    if (argc >= 2 && argv[1].type == CS_T_STR) {
        args[1] = cs_str(vm, "code");
        args[2] = cs_value_copy(argv[1]);
        nf_mset(vm, NULL, 3, args, NULL);
        cs_value_release(args[1]);
        cs_value_release(args[2]);
    } else {
        cs_value code_val = cs_str(vm, "ERROR");
        args[1] = cs_str(vm, "code");
        args[2] = code_val;
        nf_mset(vm, NULL, 3, args, NULL);
        cs_value_release(args[1]);
        cs_value_release(code_val);
    }
    
    // Capture stack trace
    cs_value stack = cs_capture_stack_trace(vm);
    args[1] = cs_str(vm, "stack");
    args[2] = stack;
    nf_mset(vm, NULL, 3, args, NULL);
    cs_value_release(args[1]);
    cs_value_release(stack);
    
    *out = err_map;
    return 0;
}

static int nf_is_error(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    if (!out) return 0;
    if (argc < 1) {
        *out = cs_bool(0);
        return 0;
    }
    
    // Error objects are maps with "msg" and "code" keys
    if (argv[0].type != CS_T_MAP) {
        *out = cs_bool(0);
        return 0;
    }
    
    cs_value args[2];
    args[0] = argv[0];
    args[1] = cs_str(vm, "msg");
    cs_value has_msg;
    nf_mhas(vm, NULL, 2, args, &has_msg);
    cs_value_release(args[1]);
    
    args[1] = cs_str(vm, "code");
    cs_value has_code;
    nf_mhas(vm, NULL, 2, args, &has_code);
    cs_value_release(args[1]);
    
    int is_err = has_msg.as.b && has_code.as.b;
    *out = cs_bool(is_err);
    return 0;
}

void cs_register_stdlib(cs_vm* vm) {
    cs_register_native(vm, "print",  nf_print,  NULL);
    cs_register_native(vm, "typeof", nf_typeof, NULL);
    cs_register_native(vm, "getenv", nf_getenv, NULL);
    cs_register_native(vm, "assert", nf_assert, NULL);
    cs_register_native(vm, "load",   nf_load,   NULL);
    cs_register_native(vm, "require", nf_require, NULL);
    cs_register_native(vm, "require_optional", nf_require_optional, NULL);
    cs_register_native(vm, "list",   nf_list,   NULL);
    cs_register_native(vm, "map",    nf_map,    NULL);
    cs_register_native(vm, "strbuf", nf_strbuf, NULL);
    cs_register_native(vm, "len",    nf_len,    NULL);
    cs_register_native(vm, "push",   nf_push,   NULL);
    cs_register_native(vm, "pop",    nf_pop,    NULL);
    cs_register_native(vm, "mget",   nf_mget,   NULL);
    cs_register_native(vm, "mset",   nf_mset,   NULL);
    cs_register_native(vm, "mhas",   nf_mhas,   NULL);
    cs_register_native(vm, "mdel",   nf_mdel,   NULL);
    cs_register_native(vm, "keys",   nf_keys,   NULL);
    cs_register_native(vm, "values", nf_values, NULL);
    cs_register_native(vm, "items",  nf_items,  NULL);
    cs_register_native(vm, "insert", nf_insert, NULL);
    cs_register_native(vm, "remove", nf_remove, NULL);
    cs_register_native(vm, "slice",  nf_slice,  NULL);
    cs_register_native(vm, "substr", nf_substr, NULL);
    cs_register_native(vm, "join",   nf_join,   NULL);
    cs_register_native(vm, "to_int", nf_to_int, NULL);
    cs_register_native(vm, "to_str", nf_to_str, NULL);
    cs_register_native(vm, "gc",     nf_gc,     NULL);
    cs_register_native(vm, "range",  nf_range,  NULL);
    
    // Type predicates
    cs_register_native(vm, "is_nil",      nf_is_nil,      NULL);
    cs_register_native(vm, "is_bool",     nf_is_bool,     NULL);
    cs_register_native(vm, "is_int",      nf_is_int,      NULL);
    cs_register_native(vm, "is_float",    nf_is_float,    NULL);
    cs_register_native(vm, "is_string",   nf_is_string,   NULL);
    cs_register_native(vm, "is_list",     nf_is_list,     NULL);
    cs_register_native(vm, "is_map",      nf_is_map,      NULL);
    cs_register_native(vm, "is_function", nf_is_function, NULL);
    
    // Math functions
    cs_register_native(vm, "abs",   nf_abs,   NULL);
    cs_register_native(vm, "min",   nf_min,   NULL);
    cs_register_native(vm, "max",   nf_max,   NULL);
    cs_register_native(vm, "floor", nf_floor, NULL);
    cs_register_native(vm, "ceil",  nf_ceil,  NULL);
    cs_register_native(vm, "round", nf_round, NULL);
    cs_register_native(vm, "sqrt",  nf_sqrt,  NULL);
    cs_register_native(vm, "pow",   nf_pow,   NULL);
    
    cs_register_native(vm, "str_find",    nf_str_find,    NULL);
    cs_register_native(vm, "str_replace", nf_str_replace, NULL);
    cs_register_native(vm, "str_split",   nf_str_split,   NULL);
    cs_register_native(vm, "path_join",   nf_path_join,   NULL);
    cs_register_native(vm, "path_dirname", nf_path_dirname, NULL);
    cs_register_native(vm, "path_basename", nf_path_basename, NULL);
    cs_register_native(vm, "path_ext",    nf_path_ext,    NULL);
    cs_register_native(vm, "fmt",         nf_fmt,         NULL);
    cs_register_native(vm, "now_ms",      nf_now_ms,      NULL);
    cs_register_native(vm, "sleep",       nf_sleep,       NULL);
    
    // String ergonomics
    cs_register_native(vm, "str_trim",       nf_str_trim,       NULL);
    cs_register_native(vm, "str_ltrim",      nf_str_ltrim,      NULL);
    cs_register_native(vm, "str_rtrim",      nf_str_rtrim,      NULL);
    cs_register_native(vm, "str_lower",      nf_str_lower,      NULL);
    cs_register_native(vm, "str_upper",      nf_str_upper,      NULL);
    cs_register_native(vm, "str_startswith", nf_str_startswith, NULL);
    cs_register_native(vm, "str_endswith",   nf_str_endswith,   NULL);
    cs_register_native(vm, "str_repeat",     nf_str_repeat,     NULL);
    
    // Data quality-of-life
    cs_register_native(vm, "map_values", nf_map_values, NULL);
    cs_register_native(vm, "copy",       nf_copy,       NULL);
    cs_register_native(vm, "deepcopy",   nf_deepcopy,   NULL);
    cs_register_native(vm, "reverse",    nf_reverse,    NULL);
    cs_register_native(vm, "reversed",   nf_reversed,   NULL);
    cs_register_native(vm, "contains",   nf_contains,   NULL);
    
    // Error objects
    cs_register_native(vm, "error",    nf_error,    NULL);
    cs_register_native(vm, "is_error", nf_is_error, NULL);
    
    // GC functions
    cs_register_native(vm, "gc_stats",  nf_gc_stats,  NULL);
    cs_register_native(vm, "gc_config", nf_gc_config, NULL);
    
    // Safety control functions
    cs_register_native(vm, "set_timeout",            nf_set_timeout,            NULL);
    cs_register_native(vm, "set_instruction_limit",  nf_set_instruction_limit,  NULL);
    cs_register_native(vm, "get_timeout",            nf_get_timeout,            NULL);
    cs_register_native(vm, "get_instruction_limit",  nf_get_instruction_limit,  NULL);
    cs_register_native(vm, "get_instruction_count",  nf_get_instruction_count,  NULL);
}
