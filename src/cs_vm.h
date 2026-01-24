#ifndef CS_VM_H
#define CS_VM_H

#include "cupidscript.h"
#include "cs_parser.h"
#include "cs_value.h"

typedef struct cs_env {
    struct cs_env* parent;
    char** keys;
    cs_value* vals;
    size_t count;
    size_t cap;
} cs_env;

struct cs_func {
    int ref;
    char** params;
    size_t param_count;
    ast* body;
    cs_env* closure;
};

struct cs_vm {
    cs_env* globals;
    char* last_error;
};

cs_value cs_value_copy(cs_value v);
void     cs_value_release(cs_value v);

void cs_register_stdlib(cs_vm* vm);

#endif

