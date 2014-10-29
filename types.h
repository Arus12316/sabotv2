#ifndef NATIVETYPES_H_
#define NATIVETYPES_H_

typedef struct pair_s pair_s;
typedef struct prop_s prop_s;
typedef struct type_s type_s;

struct pair_s {
    int optype, opatt;
    type_s *t;
};

struct prop_s
{
    char *name;
    type_s *result;
    type_s **params;
};

struct type_s {
    short npairs;
    short width;
    char *str;
    pair_s *pairs;
    prop_s *static_props;
    prop_s *instance_props;
};

extern type_s void_type;
extern type_s char_type;
extern type_s int8_type;
extern type_s uint8_type;
extern type_s int16_type;
extern type_s uint16_type;
extern type_s int32_type;
extern type_s uint32_type;
extern type_s int64_type;
extern type_s uint64_type;
extern type_s int_type;
extern type_s uint_type;
extern type_s float_type;
extern type_s double_type;

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

extern type_s *typelist[];

#endif