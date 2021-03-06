#ifndef ECS_ECS_WORLD_H
#define ECS_ECS_WORLD_H

// EcsWorld is defined in ecs_entity.h

#include "ecs_common.h"
#include "ecs_component_flag.h"
#include "ecs_event.h"

/// Initializes the world subsystem. Should not be called directly.
void ecs_world_system_init(void);

/// Creates a new EcsWorld.
EcsWorld ecs_world_init(void);

/// Frees all entities, components, and events associated with an EcsWorld, then frees the world.
EcsResult ecs_world_free(EcsWorld world);

/// Creates an entity in the given world.
EcsEntity ecs_create_entity(EcsWorld world);

/// Frees all components owned by an entity, then frees the entity.
EcsResult ecs_entity_free(EcsEntity entity);

/// Enables a previously disabled entity.
EcsResult ecs_entity_enable(EcsEntity entity);

/// Disables an enabled entity.
EcsResult ecs_entity_disable(EcsEntity entity);

/// Determines if an entity is alive, but not necessarily enabled.
bool ecs_entity_is_alive(EcsEntity entity);

/// Determines if an entity is alive and enabled.
bool ecs_entity_is_enabled(EcsEntity entity);

/// Gets all component types associated with an entity.
ComponentEnum* ecs_entity_get_components(EcsEntity entity);

/*!
    \brief Gets all component types owned by all entities on the specified world.

    \param world The world to get the components from.
    \param count A pointer filled with the number of entities on the world.
    \return An array of ComponentEnums where each index corresponds to an entity.
 */
ComponentEnum* ecs_world_get_components(EcsWorld world, int* count);

/// A flag that determines if an entity is alive.
extern ComponentFlag ecs_is_alive_flag;

/// A flag that determines if an entity is enabled.
extern ComponentFlag ecs_is_enabled_flag;

#endif