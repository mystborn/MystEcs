#ifndef ECS_ECS_WORLD_H
#define ECS_ECS_WORLD_H

// EcsWorld is defined in ecs_entity.h

#include "ecs_common.h"
#include "ecs_component_flag.h"
#include "ecs_event.h"

/**
 * Creates a new EcsWorld.
 * 
 * @return The created EcsWorld.
 */
ECS_EXPORT EcsWorld ecs_world_init(void);

/**
 * @brief Frees all entities, components, and events associated with an EcsWorld, then frees the world.
 * 
 * @param world The world to destroy.
 * @return ECS_RESULT_SUCCESS on success,
 *         ECS_RESULT_INVALID_WORLD if the world didn't exist.
 */
ECS_EXPORT EcsResult ecs_world_free(EcsWorld world);

/**
 * Creates an entity in the given world.
 * 
 * @param world The world to create the entity in.
 * @return The created entity.
 */
ECS_EXPORT EcsEntity ecs_create_entity(EcsWorld world);

/**
 * Frees all components owned by an entity, then frees the entity.
 * 
 * @remarks The entity struct doesn't have anything to free directly,
 *          but attempting to use it further will cause all sorts of
 *          problems. Once this is called, do not use it anymore.
 * 
 * @param entity The entity to free.
 * @return ECS_RESULT_SUCCESS on success,
 *         ECS_RESULT_INVALID_WORLD if the world connected to the entity no longer exists.
 *         ECS_RESULT_INVALID_ENTITY if the entity no longer exists in the world.
 */
ECS_EXPORT EcsResult ecs_entity_free(EcsEntity entity);

/**
 * Enables a previously disabled entity.
 * 
 * @param entity The entity to enable.
 * @return ECS_RESULT_SUCCESS on success,
 *         ECS_RESULT_INVALID_WORLD if the world connected to the entity no longer exists.
 *         ECS_RESULT_INVALID_ENTITY if the entity no longer exists in the world.
 *         ECS_RESULT_INVALID_STATE if the entity is already enabled.
 */
ECS_EXPORT EcsResult ecs_entity_enable(EcsEntity entity);

/**
 * Disables an enabled entity.
 * 
 * @param entity The entity to disable.
 * @return ECS_RESULT_SUCCESS on success,
 *         ECS_RESULT_INVALID_WORLD if the world connected to the entity no longer exists.
 *         ECS_RESULT_INVALID_ENTITY if the entity no longer exists in the world.
 *         ECS_RESULT_INVALID_STATE if the entity is already enabled.
 */
ECS_EXPORT EcsResult ecs_entity_disable(EcsEntity entity);

/// Determines if an entity is alive, but not necessarily enabled.
ECS_EXPORT bool ecs_entity_is_alive(EcsEntity entity);

/// Determines if an entity is alive and enabled.
ECS_EXPORT bool ecs_entity_is_enabled(EcsEntity entity);

/// Gets all component types associated with an entity.
ECS_EXPORT ComponentEnum* ecs_entity_get_components(EcsEntity entity);

/**
 * Gets all component types owned by all entities on the specified world.
 *
 * @param world The world to get the components from.
 * @param count A pointer filled with the number of entities on the world.
 * @return An array of ComponentEnums where each index corresponds to an entity.
 */
ECS_EXPORT ComponentEnum* ecs_world_get_components(EcsWorld world, int* count);

/// A flag that determines if an entity is alive.
extern ComponentFlag ecs_is_alive_flag;

/// A flag that determines if an entity is enabled.
extern ComponentFlag ecs_is_enabled_flag;

#endif