#ifndef CS_VALUE_H
#define CS_VALUE_H

#include "cupidscript.h"
#include <stddef.h>

typedef struct cs_string {
    int ref;
    size_t len;
    char* data;
} cs_string;

typedef struct cs_func cs_func;

typedef struct cs_native {
    int ref;
    cs_native_fn fn;
    void* userdata;
} cs_native;

// refcounted heap objects
cs_string* cs_str_new(const char* s);
void       cs_str_incref(cs_string* s);
void       cs_str_decref(cs_string* s);

const char* cs_type_name_impl(cs_type t);

#endif
