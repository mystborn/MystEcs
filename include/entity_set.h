#ifndef ECS_ENTITY_SET_H
#define ECS_ENTITY_SET_H

#include "ecs_common.h"
#include "ecs_world.h"
#include "component.h"

/// Sets up an EntitySet for creation.
typedef struct EntitySetBuilder EntitySetBuilder;

/// Keeps an updated set of entities with and without specific components.
typedef struct EntitySet EntitySet;

/// Initializes a new EntitySetBuilder.
EntitySetBuilder* ecs_entity_set_builder_init(void);

/// Frees an EntitySetBuilder.
void ecs_entity_set_builder_free(EntitySetBuilder* builder);

/// Adds a component type that is required for an entity to be in any sets built from the EntitySetBuilder.
void ecs_entity_set_with(EntitySetBuilder* builder, ComponentManager* manager);

/// Adds a component that cannot be owned by an entity to be in any sets built from the EntitySetBuilder.
void ecs_entity_set_without(EntitySetBuilder* builder, ComponentManager* manager);

/*!
    \brief Builds an EntitySet using the constraints set on EntitySetBuilder.

    \param builder The builder used to make the set.
    \param world The world to get the entities from.
    \param free_builder true if the function should free the builder, false otherwise.
*/
EntitySet* ecs_entity_set_build(EntitySetBuilder* builder, EcsWorld world, bool free_builder);

/// Frees an EntitySet.
void ecs_entity_set_free(EntitySet* set);

/*!
    \brief Gets an array of entities that satisfy the EntitySet conditions.

    \param set The EntitySet to get the entities from.
    \param count A pointer that is filled with the length of the entity array.
    \return An array of entities that satisfy the entity set.
 */
Entity* ecs_entity_set_get_entities(EntitySet* set, int* count);

#endif