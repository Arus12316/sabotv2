#include "types.h"
#include "parse.h"
#include <stdlib.h>
#include <string.h>

primtype_s void_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 0,
    .str = "Void",
    .pairs = NULL
};

primtype_s char_type = {
    .isresolved = true,
    .npairs = 4,
    .width = 4,
    .str = "UInt",
    .pairs = char_pairs
};

primtype_s int8_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 1,
    .str = "Int8",
    .pairs = int_pairs
};

primtype_s uint8_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 1,
    .str = "UInt8",
    .pairs = uint8_pairs
};

primtype_s int16_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 2,
    .str = "Int16",
    .pairs = uint16_pairs
};

primtype_s uint16_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 2,
    .str = "UInt16",
    .pairs = uint16_pairs
};

primtype_s int32_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 4,
    .str = "UInt64",
    .pairs = uint32_pairs
};

primtype_s uint32_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 4,
    .str = "UInt64",
    .pairs = uint64_pairs
};

primtype_s int64_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 8,
    .str = "Int64",
    .pairs = int64_pairs
};

primtype_s uint64_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 8,
    .str = "UInt64",
    .pairs = uint64_pairs
};

primtype_s int_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 4,
    .str = "Int",
    .pairs = int_pairs
};

primtype_s uint_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 4,
    .str = "UInt",
    .pairs = uint_pairs
};

primtype_s float_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 4,
    .str = "Float",
    .pairs = float_pairs
};

primtype_s double_type = {
    .isresolved = true,
    .npairs = 0,
    .width = 8,
    .str = "Double",
    .pairs = double_pairs
};

primtype_s string_type = {
    .isresolved = true,
    .npairs = 0,
    .width = -1,
    .str = "String",
    .pairs = double_pairs
};

primtype_s regex_type = {
    .isresolved = true,
    .npairs = 0,
    .width = -1,
    .str = "Regex",
    .pairs = double_pairs
};


pair_s char_pairs[] = {
    {
        .optype = TOKTYPE_ADDOP,
        .opatt = TOKATT_ADD,
        .t = &char_type,
    },
    {
        .optype = TOKTYPE_ADDOP,
        .opatt = TOKATT_SUB,
        .t = &char_type,
    },
    {
        .optype = TOKTYPE_ADDOP,
        .opatt = TOKATT_ADD,
        .t = &int_type
    },
    {
        .optype = TOKTYPE_ADDOP,
        .opatt = TOKATT_SUB,
        .t = &int_type
    }
};

pair_s int8_pairs[] = {

};

pair_s uint8_pairs[] = {
    
};

pair_s int16_pairs[] = {
    
};

pair_s uint16_pairs[] = {
    
};

pair_s int32_pairs[] = {
    
};

pair_s uint32_pairs[] = {
    
};

pair_s int64_pairs[] = {
    
};

pair_s uint64_pairs[] = {
    
};

pair_s int_pairs[] = {
    
};

pair_s uint_pairs[] = {
    
};

pair_s float_pairs[] = {
    
};

pair_s double_pairs[] = {
    
};

pair_s string_pairs[] = {

};

pair_s regex_pairs[] = {

};


primtype_s *typelist[] = {
    &void_type,
    &char_type,
    &int8_type,
    &uint8_type,
    &int16_type,
    &uint16_type,
    &int32_type,
    &uint32_type,
    &int64_type,
    &uint64_type,
    &int_type,
    &uint_type,
    &float_type,
    &double_type,
    &string_type,
    &regex_type
};

primtype_s *nextype(int i)
{
    if(i < sizeof(typelist) / sizeof(primtype_s *)) {
        return typelist[i];
    }
    return NULL;
}

