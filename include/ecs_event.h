#ifndef ECS_ECS_EVENT_H
#define ECS_ECS_EVENT_H

#include "ecs_common.h"
#include "entity.h"
#include "int_dispenser.h"

typedef struct EcsClosure {
    void* data;
    void* function;
    bool active;
} EcsClosure;

#define ecs_closure(data, function) (EcsClosure){ data, function, false }

// A C# like event.
typedef struct EcsEvent {
    IntDispenser dispenser;
    EcsClosure* subscriptions;
    int capacity;
} EcsEvent;

// Manages an event type that should be specific to each EcsWorld, but used by all of them.
typedef struct EcsEventManager {
    EcsEvent** events;
    int capacity;
    int id;
} EcsEventManager;

// Defines a new Ecs Event.
EcsEventManager* ecs_event_define(void);

// Frees an EcsEvent. Should only be used if sure that the event will never be used again.
void ecs_event_manager_free(EcsEventManager* manager);

// Subscribes a function to an Ecs Event. Returns an id that can be used to unsubscribe if needed.
int ecs_event_subscribe(EcsWorld world, EcsEventManager* manager, EcsClosure closure);

// Unsubscribes from an Ecs Event.
EcsResult ecs_event_unsubscribe(EcsWorld world, EcsEventManager* manager, int id);

// Creates and initializes a new event.
EcsEvent* ecs_event_init(void);

// Frees an event.
void ecs_event_free(EcsEvent* event);

// Subscribes a function to an event. Returns an id that can be used to unsubscribe if needed.
int ecs_event_add(EcsEvent* event, EcsClosure closure);

// Unsubscribes a function from an event.
bool ecs_event_remove(EcsEvent* event, int id);

// Triggers an event.
// event: The event to trigger.
// event_signature: The function signature of the event.
// ...: The arguments to pass to the functions subscribed to the event.
#define ecs_event_trigger(event, event_signature, ...) \
    do { \
        for(int ___ecs_event_publish_index = 0; ___ecs_event_publish_index < (event)->capacity; ++___ecs_event_publish_index) { \
            EcsClosure ___ecs_closure = (event)->subscriptions[___ecs_event_publish_index]; \
            if(___ecs_closure.active) \
                ((event_signature)(___ecs_closure.function))(___ecs_closure.data, ## __VA_ARGS__); \
        } \
    } while(0)

// Triggers an Ecs Event on the specified world.
// world: The world on which the event is triggering.
// event_manager: The Ecs Event to trigger.
// event_signature: The function signature of the event.
// ...: The arguments to pass to the functions subscribed to the event.
#define ecs_event_publish(world, event_manager, event_signature, ...) \
    do { \
        if((world) >= (event_manager)->capacity) \
            break; \
        EcsEvent* ___ecs_event = (event_manager)->events[(world)]; \
        if(___ecs_event == NULL) \
            break; \
        for(int ___ecs_event_publish_index = 0; ___ecs_event_publish_index < ___ecs_event->capacity; ++___ecs_event_publish_index) { \
            EcsClosure ___ecs_closure = (___ecs_event)->subscriptions[___ecs_event_publish_index]; \
            if(___ecs_closure.active) \
                ((event_signature)(___ecs_closure.function))(___ecs_closure.data, ## __VA_ARGS__); \
        } \
    } while(0)


#endif