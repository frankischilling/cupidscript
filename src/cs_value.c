#include "cs_value.h"
#include <stdlib.h>
#include <string.h>

static uint32_t hash_bytes(const unsigned char* data, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static uint32_t hash_u64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint32_t)(x ^ (x >> 32));
}

static char* cs_strdup(const char* s) {
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

cs_string* cs_str_new(const char* s) {
    cs_string* st = (cs_string*)calloc(1, sizeof(cs_string));
    if (!st) return NULL;
    st->ref = 1;
    st->data = cs_strdup(s ? s : "");
    st->len = st->data ? strlen(st->data) : 0;
    st->cap = st->len;
    return st;
}

cs_string* cs_str_new_take(char* owned, size_t len) {
    cs_string* st = (cs_string*)calloc(1, sizeof(cs_string));
    if (!st) { free(owned); return NULL; }
    st->ref = 1;
    if (!owned) {
        st->data = cs_strdup("");
        st->len = st->data ? strlen(st->data) : 0;
        st->cap = st->len;
        return st;
    }
    st->data = owned;
    st->len = (len == (size_t)-1) ? strlen(owned) : len;
    st->cap = st->len;
    return st;
}

void cs_str_incref(cs_string* s) {
    if (s) s->ref++;
}

void cs_str_decref(cs_string* s) {
    if (!s) return;
    s->ref--;
    if (s->ref <= 0) {
        free(s->data);
        free(s);
    }
}

const char* cs_type_name_impl(cs_type t) {
    switch (t) {
        case CS_T_NIL:    return "nil";
        case CS_T_BOOL:   return "bool";
        case CS_T_INT:    return "int";
        case CS_T_FLOAT:  return "float";
        case CS_T_STR:    return "string";
        case CS_T_LIST:   return "list";
        case CS_T_MAP:    return "map";
        case CS_T_STRBUF: return "strbuf";
        case CS_T_RANGE:  return "range";
        case CS_T_FUNC:   return "function";
        case CS_T_NATIVE: return "native";
        default:          return "unknown";
    }
}

uint32_t cs_value_hash(cs_value v) {
    switch (v.type) {
        case CS_T_NIL:
            return 0x9e3779b9u;
        case CS_T_BOOL:
            return v.as.b ? 0x85ebca6bu : 0xc2b2ae35u;
        case CS_T_INT: {
            double dv = (double)v.as.i;
            uint64_t bits = 0;
            if (dv != 0.0) memcpy(&bits, &dv, sizeof(bits));
            return hash_u64(bits);
        }
        case CS_T_FLOAT: {
            double dv = v.as.f;
            uint64_t bits = 0;
            if (dv != 0.0) memcpy(&bits, &dv, sizeof(bits));
            return hash_u64(bits);
        }
        case CS_T_STR: {
            cs_string* s = (cs_string*)v.as.p;
            if (!s || !s->data) return 0;
            return hash_bytes((const unsigned char*)s->data, s->len);
        }
        default: {
            uintptr_t p = (uintptr_t)v.as.p;
            uint64_t mix = ((uint64_t)p) ^ ((uint64_t)v.type << 32);
            return hash_u64(mix);
        }
    }
}

int cs_value_key_equals(cs_value a, cs_value b) {
    if (a.type != b.type) {
        if ((a.type == CS_T_INT && b.type == CS_T_FLOAT) || (a.type == CS_T_FLOAT && b.type == CS_T_INT)) {
            double av = (a.type == CS_T_INT) ? (double)a.as.i : a.as.f;
            double bv = (b.type == CS_T_INT) ? (double)b.as.i : b.as.f;
            return av == bv;
        }
        return 0;
    }

    switch (a.type) {
        case CS_T_NIL:
            return 1;
        case CS_T_BOOL:
            return a.as.b == b.as.b;
        case CS_T_INT:
            return a.as.i == b.as.i;
        case CS_T_FLOAT:
            return a.as.f == b.as.f;
        case CS_T_STR: {
            cs_string* sa = (cs_string*)a.as.p;
            cs_string* sb = (cs_string*)b.as.p;
            if (!sa || !sb) return sa == sb;
            if (sa->len != sb->len) return 0;
            return memcmp(sa->data, sb->data, sa->len) == 0;
        }
        default:
            return a.as.p == b.as.p;
    }
}
