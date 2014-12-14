#ifndef NATIVETYPES_H_
#define NATIVETYPES_H_

#include <stdbool.h>

typedef enum {
    CLASS_SCALAR,
    CLASS_ARRAY,
    CLASS_CLOJURE,
    CLASS_ERROR
} class_e;

typedef struct pair_s pair_s;
typedef struct prop_s prop_s;
typedef struct primtype_s primtype_s;
typedef struct type_s type_s;

struct pair_s {
    int optype, opatt;
    primtype_s *t;
};

struct prop_s
{
    char *name;
    unsigned lineno;
    primtype_s *result;
    primtype_s **params;
};

struct primtype_s {
    bool isresolved;
    union {
        char *pstr;
        primtype_s *p;
    };
    short npairs;
    short width;
    char *str;
    pair_s *pairs;
    prop_s *static_props;
    prop_s *instance_props;
    unsigned short lineno;
};

struct type_s {
    class_e cat;
    union {
        struct {
            primtype_s *val;
        } prim;
        struct {
            primtype_s *ret;
            void *param;
        } closure;
        struct {
            primtype_s *of;
            void *indeces;
        } array;
    };
};

extern primtype_s void_type;
extern primtype_s char_type;
extern primtype_s int8_type;
extern primtype_s uint8_type;
extern primtype_s int16_type;
extern primtype_s uint16_type;
extern primtype_s int32_type;
extern primtype_s uint32_type;
extern primtype_s int64_type;
extern primtype_s uint64_type;
extern primtype_s int_type;
extern primtype_s uint_type;
extern primtype_s float_type;
extern primtype_s double_type;
extern primtype_s string_type;
extern primtype_s regex_type;

extern pair_s char_pairs[];
extern pair_s int8_pairs[];
extern pair_s uint8_pairs[];
extern pair_s int16_pairs[];
extern pair_s uint16_pairs[];
extern pair_s int32_pairs[];
extern pair_s uint32_pairs[];
extern pair_s int64_pairs[];
extern pair_s uint64_pairs[];
extern pair_s int_pairs[];
extern pair_s uint_pairs[];
extern pair_s float_pairs[];
extern pair_s double_pairs[];
extern pair_s string_pairs[];
extern pair_s regex_pairs[];

extern primtype_s *typelist[];

extern primtype_s *nextype(int i);

#endif