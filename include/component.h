#ifndef ECS_COMPONENT_H
#define ECS_COMPONENT_H

#include "ecs_common.h"
#include "ecs_event.h"
#include "entity.h"
#include "component_flag.h"

/// Function definition used when a new component is created.
typedef void (*ComponentConstructor)(void*);

/// Function definition used when a component is destroyed.
typedef void (*ComponentDestructor)(void*);

struct ComponentPool;

/// Defines and handles the memory management of a component type.
typedef struct ComponentManager {
    /// The unique identifer of this component type.
    ComponentFlag flag;

/// \privatesection

    ComponentConstructor constructor;
    ComponentDestructor destructor;

    EcsEventManager* added;
    EcsEventManager* removed;

    struct ComponentPool** pools;

    int pool_count;

    // The size of the component in bytes.
    int component_size;

    int world_disposed_id;
} ComponentManager;

/*!
  \brief Used to enumerate over all active components of a specific type.


  Generally not used directly by the user, instead by the macros
  ECS_COMPONENT_ITERATE_ENABLED_START and ECS_COMPONENT_ITERATE_ENABLED_END
 */
typedef struct ComponentIterator {
/// \privatesection
    ComponentEnum flag;
    struct ComponentPool* pool;
    ComponentEnum* components;
    int index;
} ComponentIterator;

/*!
  \brief Creates a new component type.

  \param component_size The size of the component type. Used to allocate new components.
  \param constructor A function that is called when a new component is created. Can be NULL.
  \param destructor A function that is called when a component is removed. Can be NULL.
  \return A new ComponentManager
 */
ComponentManager* ecs_component_define(int component_size, ComponentConstructor constructor, ComponentDestructor destructor);

/// Frees all components owned by a ComponentManager, then frees the manager.
void ecs_component_free(ComponentManager* manager);

/*!
  \brief Creates and associates a component with an entity.

  \return A pointer to the new component. This will be one level of indirection higher than the component type.
 */
void* ecs_component_set(Entity entity, ComponentManager* manager);

/*!
  \brief Associates a component owned by a reference entity with another entity.

  \param entity The entity to add a component to.
  \param reference The entity that already owns the component.
  \param manager The component type.
 */
EcsResult ecs_component_set_same_as(Entity entity, Entity reference, ComponentManager* manager);

/// Removes a component from an entity.
EcsResult ecs_component_remove(Entity entity, ComponentManager* manager);

/// Enables a previously disabled component.
EcsResult ecs_component_enable(Entity entity, ComponentManager* manager);

/// Disables a currently enabled component.
EcsResult ecs_component_disable(Entity entity, ComponentManager* manager);

/*!
  \brief Determines if both an entity and a component owned by the entity are enabled.
  
  \return true if coth the entity and component are enabled, false otherwise.
 */
bool ecs_component_is_enabled(Entity entity, ComponentManager* manager);

/*!
  \brief Get a component associated with an entity.

  \param entity The entity that owns the component.
  \param manager The type of the component to get.
  \param component A pointer that is filled with the component value. Should be two levels of indirection higher than the actual component type.
                   (i.e. if the component type is int, the value passed to the function should be int**)
 */
EcsResult ecs_component_get(Entity entity, ComponentManager* manager, void** component);

/// Determines if a component is associated with an entity.
bool ecs_component_exists(Entity entity, ComponentManager*);

/*!
  \brief Gets all created components, regardless if they're enabled or not.

  \param world The world to get all of the components from.
  \param manager The type of the component to get.
  \param count An int pointer that is filled with the number of components.
  \return An array that holds the components. Do not free this array.
 */
void* ecs_component_get_all(EcsWorld world, ComponentManager* manager, int* count);

// Todo: Consider inlining these
//       Pros: Faster iteration
//       Cons: Must expose ComponentPool implementation

/*!
  \brief Initializes a ComponentIterator for a specific component type.

  \param world The world to get the components from.
  \param manager The type of component to iterate.
  \param iterator An iterator to initialize.
 */
void ecs_component_iter_enabled_start(EcsWorld world, ComponentManager* manager, ComponentIterator* iterator);

/*!
  \brief Moves the ComponentIterator to the next active component, storing the value in result.

  \param iterator An iterator to move forward.
  \param result A pointer that is filled with the result. Should be two levels of indirection higher than the actual component type.
  \result true if the iterator moved forward, false if there are no more components.
 */
bool ecs_component_iter_enabled_next(ComponentIterator* iterator, void** result);

/// Releases resources used by the ComponentIterator. Does not free the iterator.
void ecs_component_iter_enabled_end(ComponentIterator* iterator);

/// Gets an EcsEventManager that is triggered when the specified component type is added to an entity.
static inline EcsEventManager* ecs_component_get_added_event(ComponentManager* manager) {
    if(manager->added == NULL)
        manager->added = ecs_event_define();

    return manager->added;
}

/// Gets an EcsEventManager that is triggered when the specified component type is removed from an entity.
static inline EcsEventManager* ecs_component_get_removed_event(ComponentManager* manager) {
    if(manager->removed == NULL)
        manager->removed = ecs_event_define();

    return manager->removed;
}

/*!
  \brief Starts a block that can be used to iterate over all components of a specific type.
         The block must be closed with ECS_COMPONENT_ITERATE_ENABLED_END.
  
  \param world The world to get the components from.
  \param manager The type of component to iterate.
  \param result A pointer that is filled with the component value. Should be two levels of indirection higher than the actual component type.
 */
#define ECS_COMPONENT_ITERATE_ENABLED_START(world, manager, result) \
    { \
        ComponentIterator ___ecs_component_iterator; \
        ecs_component_iter_enabled_start((world), (manager), &___ecs_component_iterator); \
        while(ecs_component_iter_enabled_next(&___ecs_component_iterator, (result))) {

/// Closes a block previously started with ECS_COMPONENT_ITERATE_ENABLED_START.
#define ECS_COMPONENT_ITERATE_ENABLED_END() \
        } \
        ecs_component_iter_enabled_end(&___ecs_component_iterator); \
    }

#endif