#ifndef ECS_COMPONENT_FLAG_H
#define ECS_COMPONENT_FLAG_H

#include "ecs_common.h"

// Marks a component type with a unique id.
typedef long long ComponentFlag;

// Refers to an invalid ComponentFlag.
#define COMPONENT_FLAG_INVALID_MASK 0xFFFFFFFFFFFFFFFF

// The mask of the index portion of a ComponentFlag.
#define COMPONENT_FLAG_INDEX_MASK   0xFFFFFFFF00000000

// The maks of the bit portion of a ComponentFlag.
#define COMPONENT_FLAG_BIT_MASK     0x00000000FFFFFFFF

// Gets the index portion of a ComponentFlag.
#define COMPONENT_FLAG_INDEX(f) (((f) & COMPONENT_FLAG_INDEX_MASK) >> 32)

// Gets the bit portion of a ComponentFlag.
#define COMPONENT_FLAG_BIT(f)   ((f) & COMPONENT_FLAG_BIT_MASK)

// Internal use.
extern ComponentFlag ___ecs_last_component_flag;

// Internal use.
static inline ComponentFlag ecs_component_flag_get() {
    ComponentFlag flag = ___ecs_last_component_flag;
    ___ecs_last_component_flag = (flag & COMPONENT_FLAG_BIT_MASK) != 0x80000000 ? ((flag & COMPONENT_FLAG_INDEX_MASK) | ((flag & COMPONENT_FLAG_BIT_MASK) << 1)) :
                                                                                  ((flag & COMPONENT_FLAG_INDEX_MASK) + 0x100000000) + 1;

    return flag;
}

// Contains a set of ComponentFlags
typedef struct ComponentEnum {
    unsigned int* bit_array;
    int count;
} ComponentEnum;

// The default ComponentEnum. Can be used as an r-value.
extern ComponentEnum COMPONENT_ENUM_DEFAULT;

// Initializes a ComponentEnum for use.
static inline void ecs_component_enum_init(ComponentEnum* cenum) {
    cenum->bit_array = NULL;
    cenum->count = 0;
}

// Frees the resources held by a ComponentEnum. Does not free the ComponentEnum.
static inline void ecs_component_enum_free_resources(ComponentEnum* cenum) {
    if(cenum->bit_array != NULL)
        ecs_free(cenum->bit_array);
}

// Determines if a ComponentEnum has a specific ComponentFlag.
static inline bool ecs_component_enum_get_flag(ComponentEnum* cenum, ComponentFlag flag) {
    return COMPONENT_FLAG_INDEX(flag) < cenum->count && (cenum->bit_array[COMPONENT_FLAG_INDEX(flag)] & COMPONENT_FLAG_BIT(flag)) != 0;
}

// Sets a ComponentFlag in a ComponentEnum to the specified value.
static inline void ecs_component_enum_set_flag(ComponentEnum* cenum, ComponentFlag flag, bool value) {
    int index = COMPONENT_FLAG_INDEX(flag);
    ECS_ARRAY_RESIZE_DEFAULT(cenum->bit_array, cenum->count, index + 1, sizeof(unsigned int), 0);
    if(value)
        cenum->bit_array[index] |= COMPONENT_FLAG_BIT(flag);
    else
        cenum->bit_array[index] &= ~COMPONENT_FLAG_BIT(flag);
}

// Determines if a ComponentEnum is a superset of another ComponentEnum.
static inline bool ecs_component_enum_contains_enum(ComponentEnum* cenum, ComponentEnum* filter) {
    if(filter->count != 0) {
        for(int i = 0; i < filter->count; ++i) {
            unsigned int part = filter->bit_array[i];
            if(part != 0 && (i >= cenum->count || (cenum->bit_array[i] & part) != part))
                return false;
        }
    }

    return true;
}

// Determines if a ComponentEnum does not intersect at all with another ComponentEnum.
static inline bool ecs_component_enum_not_contains_enum(ComponentEnum* cenum, ComponentEnum* filter) {
    if(filter->count != 0) {
        for(int i = 0; i < filter->count; ++i) {
            unsigned int part = filter->bit_array[i];
            if(part != 0 && i < cenum->count && (cenum->bit_array[i] & part) != 0u)
                return false;
        }
    }

    return true;
}

// Creates a new ComponentEnum and copies the values from another ComponentEnum.
static inline ComponentEnum ecs_component_enum_copy(ComponentEnum* src) {
    ComponentEnum result;
    result.bit_array = ecs_malloc(src->count * sizeof(unsigned int));
    ecs_memcpy(result.bit_array, src->bit_array, src->count * sizeof(unsigned int));
    result.count = src->count;
    return result;
}

// Clears the information stored by a ComponentEnum.
static inline void ecs_component_enum_clear(ComponentEnum* cenum) {
    ecs_memset(cenum->bit_array, 0, cenum->count);
}

#endif