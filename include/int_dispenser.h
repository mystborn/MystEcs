/*!
 * @file
 *
 * \brief A utility struct that gets open ids.
 *
 * This header defines a utility struct that gets open ids.
 * Typically used to keep elements in an array as close together as possible
 * without sacrificing much performance.
 */
#ifndef ECS_INT_DISPENSER
#define ECS_INT_DISPENSER

#include "ecs_common.h"

/// A helper struct that efficiently handles open array indexes.
typedef struct EcsIntDispenser {
/// \privatesection
    int* free_ints;
    int free_count;
    int free_capacity;
    int total;
} EcsIntDispenser;

/// Initializes an int dispenser.
static inline void ecs_dispenser_init(EcsIntDispenser* id) {
    id->free_ints = NULL;
    id->free_count = 0;
    id->free_capacity = 0;
    id->total = 0;
}

/// Initializes an int dispenser with a given starting point.
static inline void ecs_dispenser_init_start(EcsIntDispenser* id, int start) {
    id->free_ints = NULL;
    id->free_count = 0;
    id->free_capacity = 0;
    id->total = start;
}

/// Frees all resources owned by an int dispenser. Does not free the dispenser.
static inline void ecs_dispenser_free_resources(EcsIntDispenser* id) {
    if(id->free_ints != NULL)
        ecs_free(id->free_ints);
}

/// Frees all resources owned by an int dispenser and frees the dispenser.
static inline void ecs_dispenser_free(EcsIntDispenser* id) {
    if(id->free_ints != NULL)
        ecs_free(id->free_ints);
    ecs_free(id);
}

/// Gets an open index.
static inline int ecs_dispenser_get(EcsIntDispenser* id) {
    if(id->free_count == 0)
        return id->total++;
    else
        return id->free_ints[--id->free_count];
}

/// Releases an index to be used later.
static inline void ecs_dispenser_release(EcsIntDispenser* id, int value) {
    ECS_ARRAY_RESIZE(id->free_ints, id->free_capacity, id->free_count + 1, sizeof(*id->free_ints));
    id->free_ints[id->free_count++] = value;
}

#endif