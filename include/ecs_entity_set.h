/*!
 * @file
 *
 * \brief A data structure that can be used to get all components with and without specific components.
 *
 * This header defines a data structure that can be used to get all entities with and without specific components.
 * It's primarily used by EcsEntitySystem, but it can be used directly for more performance, or for
 * specific operations where you need all of the entities at once.
 */
#ifndef ECS_ENTITY_SET_H
#define ECS_ENTITY_SET_H

#include "ecs_common.h"
#include "ecs_world.h"
#include "ecs_component.h"

/// Sets up an EcsEntitySet for creation.
typedef struct EcsEntitySetBuilder EcsEntitySetBuilder;

/// Keeps an updated set of entities with and without specific components.
typedef struct EcsEntitySet EcsEntitySet;

typedef int (*EcsEntityComparer)(EcsEntity left, EcsEntity right, void* context);

/// Initializes a new EcsEntitySetBuilder.
ECS_EXPORT EcsEntitySetBuilder* ecs_entity_set_builder_init(void);

/// Frees an EcsEntitySetBuilder.
ECS_EXPORT void ecs_entity_set_builder_free(EcsEntitySetBuilder* builder);

/// Adds a component type that is required for an entity to be in any sets built from the EcsEntitySetBuilder.
ECS_EXPORT void ecs_entity_set_with(EcsEntitySetBuilder* builder, EcsComponentManager* component);

/// Adds a component that cannot be owned by an entity to be in any sets built from the EcsEntitySetBuilder.
ECS_EXPORT void ecs_entity_set_without(EcsEntitySetBuilder* builder, EcsComponentManager* component);

/**
 * Adds a set of components of which at least one is required for an entity to be in any sets
 * from the EcsEntitySetBuilder.
 * 
 * @param builder The builder instance that will be used to construct the EcsEntitySet.
 * @param components A list of components that an entity will need at least one of to be considered in
 *                   the set.
 * @param count The number of components in the list.
*/
ECS_EXPORT void ecs_entity_set_with_any(EcsEntitySetBuilder* builder, EcsComponentManager** components, int count);


ECS_EXPORT void ecs_entity_set_sorted(EcsEntitySetBuilder* builder, EcsEntityComparer comparer);

/**
 * Builds an EcsEntitySet using the constraints set on EcsEntitySetBuilder.
 *
 * @param builder The builder used to make the set.
 * @param world The world to get the entities from.
 * @param free_builder true if the function should free the builder, false otherwise.
*/
ECS_EXPORT EcsEntitySet* ecs_entity_set_build(EcsEntitySetBuilder* builder, EcsWorld world, bool free_builder);

/// Frees an EcsEntitySet.
ECS_EXPORT void ecs_entity_set_free(EcsEntitySet* set);

/**
 * Gets an array of entities that satisfy the EcsEntitySet conditions.
 *
 * @param set The EcsEntitySet to get the entities from.
 * @param count A pointer that is filled with the length of the entity array.
 * @return An array of entities that satisfy the entity set.
 */
ECS_EXPORT EcsEntity* ecs_entity_set_get_entities(EcsEntitySet* set, int* count);

#endif