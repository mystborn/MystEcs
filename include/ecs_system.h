#ifndef ECS_ECS_SYSTEM_H
#define ECS_ECS_SYSTEM_H

#include <stdarg.h>

#include "ecs_common.h"
#include "component.h"
#include "entity.h"

// The base type of an Ecs System.
typedef struct EcsSystem EcsSystem;

// A system that iterates over all active components of a specific type.
typedef struct EcsComponentSystem EcsComponentSystem;

// A system that iterates over all active entities with set of components.
typedef struct EcsEntitySystem EcsEntitySystem;

// A system that calls a function once per update.
typedef struct EcsActionSystem EcsActionSystem;

// A system that updates any number of subsystems on update.
typedef struct EcsSequentialSystem EcsSequentialSystem;

// A function that is called before a system update.
typedef void (*EcsSystemPreupdate)(EcsSystem*, float);

// A function that is called after a system update.
typedef void (*EcsSystemPostupdate)(EcsSystem*, float);

// A function that is called when an EcsComponentSystem updates.
typedef void (*EcsSystemUpdateComponent)(EcsComponentSystem*, float, void*);

// A function that is called when an EcsEntitySystem updates.
typedef void (*EcsSystemUpdateEntity)(EcsEntitySystem*, float, Entity);

// A function that is called when an EcsActionSystem updates.
typedef void (*EcsSystemUpdateAction)(EcsActionSystem*, float);

// Tags each system with its respective type.
typedef enum EcsSystemType {
    ECS_SYSTEM_TYPE_COMPONENT,
    ECS_SYSTEM_TYPE_ENTITY,
    ECS_SYSTEM_TYPE_SEQUENTIAL,
    ECS_SYSTEM_TYPE_ACTION
} EcsSystemType;

struct EcsSystem {
    EcsSystemPreupdate preupdate;
    EcsSystemPostupdate postupdate;
    EcsEvent* dispose;
    EcsSystemType type;
    bool enabled;
};

struct EcsComponentSystem {
    EcsSystem base;
    ComponentManager* manager;
    EcsSystemUpdateComponent update;
    EcsWorld world;
};

struct EcsEntitySystem {
    EcsSystem base;
    ComponentEnum with;
    ComponentEnum without;
    EcsSystemUpdateEntity update;
    EcsWorld world;
};

struct EcsActionSystem {
    EcsSystem base;
    EcsSystemUpdateAction update;
};

struct EcsSequentialSystem {
    EcsSystem base;
    EcsSystem** systems;
    int count;
    bool free_children;
};

// Initializes an EcsSystem. Should not be called directly.
void ecs_system_init(EcsSystem* system, EcsSystemType type, EcsSystemPreupdate preupdate, EcsSystemPostupdate postupdate);

// Frees all of the resources held by a system, and calls it's dispose event. Does not free the system.
void ecs_system_free_resources(EcsSystem* system);

// Enables a previously disabled system. Returns true if the system was successfully enabled.
bool ecs_system_enable(EcsSystem* system);

// Disables an enabled system. Returns true if the system was successfully disabled.
bool ecs_system_disable(EcsSystem* system);

// Initializes an EcsComponentSystem.
// system: The EcsComponentSystem to initialize.
// world: The world to get the components from.
// component_type: The type of the component to get.
// update: The function to call each frame. Can be NULL.
// preupdate: The function to call before each update. Can be NULL.
// postupdate: The function to call after each update. Can be NULL.
void ecs_component_system_init(EcsComponentSystem* system, 
                               EcsWorld world, 
                               ComponentManager* component_type, 
                               EcsSystemUpdateComponent update, 
                               EcsSystemPreupdate preupdate, 
                               EcsSystemPostupdate postupdate);

// Initializes an EcsEntitySystem.
// system: The EcsEntitySystem to initialize.
// world: The world to get the entities from.
// with: The components that each entity must have.
// without: The components that each entity must not have.
// update: The function to call each frame. Can be NULL.
// preupdate: The function to call before each update. Can be NULL.
// postupdate: The function to call after each update. Can be NULL.
void ecs_entity_system_init(EcsEntitySystem* system, 
                            EcsWorld world, 
                            ComponentEnum* with, 
                            ComponentEnum* without, 
                            EcsSystemUpdateEntity update, 
                            EcsSystemPreupdate preupdate, 
                            EcsSystemPostupdate postupdate);

// Initializes an EcsActionSystem.
// system: The EcsActionSystem to initialize.
// update: The function to call each frame. Can be NULL.
// preupdate: The function to call before each update. Can be NULL.
// postupdate: The function to call after each update. Can be NULL.
void ecs_action_system_init(EcsActionSystem* system, 
                            EcsSystemUpdateAction update, 
                            EcsSystemPreupdate preupdate, 
                            EcsSystemPostupdate postupdate);

// Initializes an EcsSequentialSystem with the children systems provided using varargs.
// system: The EcsSequentialSystem to initialize.
// preupdate: The function to call before each update. Can be NULL.
// postupdate: The function to call after each update. Can be NULL.
// free_children: Determines if the children systems are freed with 
//                their resources when the sequential system is freed.
// count: The number of children systems given to this function.
// ...: The children systems to add to the sequential system.
void ecs_sequential_system_init(EcsSequentialSystem* system, 
                                EcsSystemPreupdate preupdate, 
                                EcsSystemPostupdate postupdate,
                                bool free_children,
                                int count, 
                                ...);

// Initializes an EcsSequentialSystem with the children systems provided using an array. The values are copied. 
// system: The EcsSequentialSystem to initialize.
// preupdate: The function to call before each update. Can be NULL.
// postupdate: The function to call after each update. Can be NULL.
// free_children: Determines if the children systems are freed with 
//                their resources when the sequential system is freed.
// count: The number of children systems inside of the systems array.
// systems: The systems to add to the sequential system. 
//          The values are copied from the array, so it can be safely freed after the function has been called.
void ecs_sequential_system_init_array(EcsSequentialSystem* system, 
                                      EcsSystemPreupdate preupdate, 
                                      EcsSystemPostupdate postupdate,
                                      bool free_children,
                                      int count,
                                      EcsSystem** systems);

// Initializes an EcsSequentialSystem with the children provided by a va_list.
// system: The EcsSequentialSystem to initialize.
// preupdate: The function to call before each update. Can be NULL.
// postupdate: The function to call after each update. Can be NULL.
// free_children: Determines if the children systems are freed with 
//                their resources when the sequential system is freed.
// count: The number of children systems inside of the va_list.
// systems: A va_list containing children systems to add to the sequential system.
void ecs_sequential_system_init_list(EcsSequentialSystem* system, 
                                     EcsSystemPreupdate preupdate, 
                                     EcsSystemPostupdate postupdate,
                                     bool free_children,
                                     int count,
                                     va_list list);

// Updates a system.
// system: The system to update.
// delta_time: The delta time since the last update to pass to the update function.
void ecs_system_update(EcsSystem* system, float delta_time);

#endif