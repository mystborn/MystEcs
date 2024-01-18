#ifndef ECS_ECS_EVENT_H
#define ECS_ECS_EVENT_H

#include "ecs_common.h"
#include "ecs_entity.h"
#include "ecs_int_dispenser.h"

/// A very simple closure implementation that has an environment and a function.
typedef struct EcsClosure {
    /// The caller or environment of the closure.
    void* data;

    /**
     * The function pointer that the closure will invoke.
     * 
     * Technically this is not proper C, as a function pointer is not
     * guaranteed to fit in a void*, but it will on most modern
     * systems, which is the target audience of this library,
     * so this is fine. This is valid on POSIX compliant systems.
     */
    void* function;

/// @privatesection
    bool active;
} EcsClosure;

/// Creates a new EcsClosure.
#define ecs_closure(data, function) (EcsClosure){ data, function, false }

/// A C# like event.
typedef struct EcsEvent {
/// @privatesection
    EcsIntDispenser dispenser;
    EcsClosure* subscriptions;
    int capacity;
} EcsEvent;

/// Manages an event type that should be specific to each EcsWorld, but used by all of them.
typedef struct EcsEventManager {
/// \privatesection
    EcsEvent** events;
    int capacity;
    int id;
} EcsEventManager;

/// Defines a new event manager.
EcsEventManager* ecs_event_define(void);

/**
 * Frees all events owned by an event manager, then frees the manager.
 * Should only be used if the event will never be used again.
 *
 * @param manager The event manager to free.
 */
void ecs_event_manager_free(EcsEventManager* manager);

/**
 * Subscribes a function to an event manager.
 *  
 * @param world The world to add the function to.
 * @param manager The event manager to subscribe to.
 * @param closure The function to subscribe.
 * @return An id that can be used to unsubscribe if needed.
 */
int ecs_event_subscribe(EcsWorld world, EcsEventManager* manager, EcsClosure closure);

/**
 * Unsubscribes a function from an event manager.
 *
 * @param world The world that the function was added to.
 * @param manager The manager to unsubscribe from.
 * @param id The id that was previously returned when ecs_event_subscribe was called.
 */
EcsResult ecs_event_unsubscribe(EcsWorld world, EcsEventManager* manager, int id);

/// Creates and initializes a new event.
EcsEvent* ecs_event_init(void);

/**
 * Frees an event.
 * 
 * @param event The event to free.
 */
void ecs_event_free(EcsEvent* event);

/**
 * Adds a function to an event.
 *
 * @param event The event to add to.
 * @param closure The function to add.
 * @return An id that can be used to remove the function if needed.
 */
int ecs_event_add(EcsEvent* event, EcsClosure closure);

/**
 * Removes a function from an event.
 *
 * @param event The event to remove from.
 * @param id The id that was returned when ecs_event_add was called.
 * @return true if the id was successfully removed, false otherwise.
 */
bool ecs_event_remove(EcsEvent* event, int id);

/**
 * Triggers an event.
 *
 * @param event The event to trigger.
 * @param event_signature The function signature of the event.
 * @param ... The arguments to pass to the functions added to the event.
 */
#define ecs_event_trigger(event, event_signature, ...) \
    do { \
        for(int ecs_event_publish_index = 0; ecs_event_publish_index < (event)->capacity; ++ecs_event_publish_index) { \
            EcsClosure ecs_closure = (event)->subscriptions[ecs_event_publish_index]; \
            if(ecs_closure.active) \
                ((event_signature)(ecs_closure.function))(ecs_closure.data, ## __VA_ARGS__); \
        } \
    } while(0)

/**
 * Triggers an event manager for the specified world.
 *
 * @param world The world to trigger the manager for.
 * @param event_manager The event manager to trigger.
 * @param event_signature The function signature of the event.
 * @param ... The arguments to pass to the functions subscribed to the event manager.
 */
#define ecs_event_publish(world, event_manager, event_signature, ...) \
    do { \
        if((world) >= (event_manager)->capacity) \
            break; \
        EcsEvent* ecs_event = (event_manager)->events[(world)]; \
        if(ecs_event == NULL) \
            break; \
        for(int ecs_event_publish_index = 0; ecs_event_publish_index < ecs_event->capacity; ++ecs_event_publish_index) { \
            EcsClosure ecs_closure = (ecs_event)->subscriptions[ecs_event_publish_index]; \
            if(ecs_closure.active) \
                ((event_signature)(ecs_closure.function))(ecs_closure.data, ## __VA_ARGS__); \
        } \
    } while(0)


#endif