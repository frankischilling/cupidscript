#include "cs_vm.h"
#include <stdio.h>
#include <stdlib.h>

static int nf_print(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
    (void)vm; (void)ud;
    for (int i = 0; i < argc; i++) {
        switch (argv[i].type) {
            case CS_T_NIL:  fputs("nil", stdout); break;
            case CS_T_BOOL: fputs(argv[i].as.b ? "true" : "false", stdout); break;
            case CS_T_INT:  fprintf(stdout, "%lld", (long long)argv[i].as.i); break;
            case CS_T_STR:  fputs(((cs_string*)argv[i].as.p)->data, stdout); break;
            default:        fputs("<obj>", stdout); break;
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

void cs_register_stdlib(cs_vm* vm) {
    cs_register_native(vm, "print",  nf_print,  NULL);
    cs_register_native(vm, "typeof", nf_typeof, NULL);
    cs_register_native(vm, "getenv", nf_getenv, NULL);
    cs_register_native(vm, "assert", nf_assert, NULL);
}
