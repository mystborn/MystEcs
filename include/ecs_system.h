#ifndef ECS_ECS_SYSTEM_H
#define ECS_ECS_SYSTEM_H

#include <stdarg.h>

#include "ecs_common.h"
#include "ecs_component.h"
#include "ecs_entity.h"
#include "ecs_entity_set.h"

/// The base type of an Ecs System.
typedef struct EcsSystem EcsSystem;

/// A system that iterates over all active components of a specific type.
typedef struct EcsComponentSystem EcsComponentSystem;

/// A system that iterates over all active entities with set of components.
typedef struct EcsEntitySystem EcsEntitySystem;

/// A system that calls a function once per update.
typedef struct EcsActionSystem EcsActionSystem;

/// A system that calls a function that includes an environment pointer once per update.
typedef struct EcsClosureSystem EcsClosureSystem;

/// A system that updates any number of subsystems on update.
typedef struct EcsSequentialSystem EcsSequentialSystem;

/**
 * A function that is called before a system update.
 * 
 * @param system The system calling the preupdate function.
 * @param delta_time The time since the last update in ms.
*/
typedef void (*EcsSystemPreupdate)(EcsSystem* system, float delta_time);

/**
 * A function that is called after a system update.
 * 
 * @param system The system calling the postupdate function.
 * @param delta_time The time since the last update in ms.
*/
typedef void (*EcsSystemPostupdate)(EcsSystem* system, float delta_time);

/**
 * A function that is called when an EcsComponentSystem updates.
 * 
 * @param system The system calling the update function.
 * @param delta_time The time since the last update in ms.
 * @param component The component to update. This will be one level of indirection higher than the component type.
 *                  In other words, if the component is `int`, component will be `int*`.
 */
typedef void (*EcsSystemUpdateComponent)(EcsComponentSystem* system, float delta_time, void* component);

/**
 * A function that is called when an EcsEntitySystem updates.
 * 
 * @param system The system calling the update function.
 * @param delta_time The time since the last update in ms.
 * @param entity The entity to update.
 */
typedef void (*EcsSystemUpdateEcsEntity)(EcsEntitySystem* system, float delta_time, EcsEntity entity);

/**
 * A function that is called when an EcsActionSystem updates.
 * 
 * @param system The system calling the update function.
 * @param delta_time The time since the last update in ms.
 */
typedef void (*EcsSystemUpdateAction)(EcsActionSystem* system, float delta_time);

/**
 * A function that is called when an EcsClosureSystem updates.
 * 
 * @param system The system calling the update function.
 * @param delta_time The time since the last update in ms.
 * @param context The environment or context of the closure.
 */
typedef void (*EcsSystemUpdateClosure)(EcsClosureSystem* system, float delta_time, void* context);

/// Tags each system with its respective type.
typedef enum EcsSystemType {
    /// An EcsComponentSystem
    ECS_SYSTEM_TYPE_COMPONENT,

    /// An EcsEntitySystem
    ECS_SYSTEM_TYPE_ENTITY,

    /// An EcsSequentialSystem
    ECS_SYSTEM_TYPE_SEQUENTIAL,

    /// An EcsActionSystem
    ECS_SYSTEM_TYPE_ACTION,

    /// An EcsClosureSystem
    ECS_SYSTEM_TYPE_CLOSURE
} EcsSystemType;

struct EcsSystem {
/// @privatesection
    EcsSystemPreupdate preupdate;
    EcsSystemPostupdate postupdate;
    EcsEvent* dispose;
    EcsSystemType type;
    bool enabled;
};

struct EcsComponentSystem {
/// @privatesection
    EcsSystem base;
    EcsComponentManager* manager;
    EcsSystemUpdateComponent update;
    EcsWorld world;
};

struct EcsEntitySystem {
/// @privatesection
    EcsSystem base;
    EcsEntitySet* entities;
    EcsSystemUpdateEcsEntity update;
    EcsWorld world;
};

struct EcsActionSystem {
/// @privatesection
    EcsSystem base;
    EcsSystemUpdateAction update;
};

struct EcsClosureSystem {
    EcsSystem base;
    void* context;
    EcsSystemUpdateClosure update;
};

struct EcsSequentialSystem {
/// @privatesection
    EcsSystem base;
    EcsSystem** systems;
    int count;
    bool free_children;
};

/** 
 * Initializes an EcsSystem. Should not be called directly.
 * 
 * @param system The system to initialize.
 * @param type The type of ecs system being initialized.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 */
ECS_EXPORT void ecs_system_init(
    EcsSystem* system,
    EcsSystemType type,
    EcsSystemPreupdate preupdate,
    EcsSystemPostupdate postupdate);

/// Frees all of the resources held by a system, and calls it's dispose event. Does not free the system.
ECS_EXPORT void ecs_system_free_resources(EcsSystem* system);

/// Enables a previously disabled system. Returns true if the system was successfully enabled.
ECS_EXPORT bool ecs_system_enable(EcsSystem* system);

/// Disables an enabled system. Returns true if the system was successfully disabled.
ECS_EXPORT bool ecs_system_disable(EcsSystem* system);

/// Gets the dispose event from a system. Can be used with 
/// any type as long as its first member is a system.
#define ecs_system_get_dispose_event(system) ((EcsSystem*)(system))->dispose

/**
 * Initializes a component system.
 *
 * @param system The component system to initialize.
 * @param world The to get the components from.
 * @param component_type The type of the component to get.
 * @param update The function to call each frame. Can be NULL.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 */
ECS_EXPORT void ecs_component_system_init(
    EcsComponentSystem* system, 
    EcsWorld world, 
    EcsComponentManager* component_type, 
    EcsSystemUpdateComponent update, 
    EcsSystemPreupdate preupdate, 
    EcsSystemPostupdate postupdate);

/**
 * Initializes an entity system.
 *
 * @param system The entity system to initialize.
 * @param world The world to get the entities from.
 * @param builder The builder that defines the components required of the entity to be processed during the system update.
 * @param free_builder Determines if the function frees the builder when it's finished using it.
 * @param update The function to call each frame. Can be NULL.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 */
ECS_EXPORT void ecs_entity_system_init(
    EcsEntitySystem* system, 
    EcsWorld world,
    EcsEntitySetBuilder* builder,
    bool free_builder,
    EcsSystemUpdateEcsEntity update, 
    EcsSystemPreupdate preupdate,
    EcsSystemPostupdate postupdate);

/**
 * Initializes an action system.
 *
 * @param system The action system to initialize.
 * @param update The function to call each frame. Can be NULL.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 */
ECS_EXPORT void ecs_action_system_init(
    EcsActionSystem* system, 
    EcsSystemUpdateAction update, 
    EcsSystemPreupdate preupdate, 
    EcsSystemPostupdate postupdate);

ECS_EXPORT void ecs_closure_system_init(
    EcsClosureSystem* system,
    EcsSystemUpdateClosure update,
    EcsSystemPreupdate preupdate,
    EcsSystemPostupdate postupdate);

/**
 * Initializes a sequential system with the children systems provided using varargs.
 *
 * @param system The sequential system to initialize.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 * @param free_children Determines if the children systems are freed with 
 *                      their resources when the sequential system is freed.
 * @param count The number of children systems given to this function.
 * @param ... The children systems to add to the sequential system.
 */
ECS_EXPORT void ecs_sequential_system_init(
    EcsSequentialSystem* system, 
    EcsSystemPreupdate preupdate, 
    EcsSystemPostupdate postupdate,
    bool free_children,
    int count, 
    ...);

/**
 * Initializes a sequential system with the children systems provided using an array. The values are copied.
 *
 * @param system The sequential system to initialize.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 * @param free_children Determines if the children systems are freed with 
 *                      their resources when the sequential system is freed.
 * @param count The number of children systems inside of the systems array.
 * @param systems The systems to add to the sequential system. 
 *                The values are copied from the array, so it can be safely freed after the function has been called.
 */
ECS_EXPORT void ecs_sequential_system_init_array(
    EcsSequentialSystem* system, 
    EcsSystemPreupdate preupdate, 
    EcsSystemPostupdate postupdate,
    bool free_children,
    int count,
    EcsSystem** systems);

/**
 * Initializes a sequential system with the children provided by a va_list.
 *
 * @param system The sequential system to initialize.
 * @param preupdate The function to call before each update. Can be NULL.
 * @param postupdate The function to call after each update. Can be NULL.
 * @param free_children Determines if the children systems are freed with 
 *                      their resources when the sequential system is freed.
 * @param count The number of children systems inside of the va_list.
 * @param list A va_list containing children systems to add to the sequential system.
 */
ECS_EXPORT void ecs_sequential_system_init_list(
    EcsSequentialSystem* system, 
    EcsSystemPreupdate preupdate, 
    EcsSystemPostupdate postupdate,
    bool free_children,
    int count,
    va_list list);

/**
 * Updates a system.
 *
 * @param system The system to update.
 * @param delta_time The time since the last update.
 */
ECS_EXPORT void ecs_system_update(EcsSystem* system, float delta_time);

#endif