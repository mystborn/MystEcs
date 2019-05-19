#include "ecs_event.h"
#include "ecs_array.h"
#include "ecs_messages.h"

struct EcsEventSystem {
    IntDispenser dispenser;
    EcsEventManager* event_managers;
    int capacity;
};

static struct EcsEventSystem event_system;
static EcsEventManager* ___ecs_manager = NULL;

EcsEvent* ecs_world_disposed;

static void on_world_disposed(EcsWorldDisposedMessage* message) {
    for(int i = 0; i < event_system.capacity; i++) {
        if(event_system.event_managers[i].active) {
            EcsEventManager* manager = event_system.event_managers + i;
            if(message->world < manager->capacity && manager->events[message->world] != NULL) {
                ecs_event_free(manager->events[message->world]);
                manager->events[message->world] = NULL;
            }
        }
    }
}

void ecs_event_system_init(void) {
    ecs_dispenser_init(&event_system.dispenser);
    event_system.event_managers = NULL;
    event_system.capacity = 0;

    ecs_world_disposed = ecs_event_init();
    ecs_event_add(ecs_world_disposed, on_world_disposed);

#if ECS_DEBUG
    puts("Event System Initialized");
#endif
}

EcsEventManager* ecs_event_define(void) {
    int id = ecs_dispenser_get(&event_system.dispenser);
    ECS_ARRAY_RESIZE(event_system.event_managers, event_system.capacity, id, sizeof(EcsEventManager));

    EcsEventManager* manager = event_system.event_managers + id;

    manager->events = NULL;
    manager->capacity = 0;
    manager->id = id;
    manager->active = true;

    return manager;
}

void ecs_event_manager_free(EcsEventManager* manager) {
    if(manager->events != NULL) {
        for(int i = 0; i < manager->capacity; ++i) {
            if(manager->events[i] != NULL)
                ecs_event_free(manager->events[i]);
        }
        ecs_free(manager->events);
    }

    manager->active = false;
    ecs_dispenser_release(&event_system.dispenser, manager->id);
}

int ecs_event_subscribe(EcsWorld world, EcsEventManager* manager, void* function) {
    ECS_ARRAY_RESIZE_DEFAULT(manager->events, manager->capacity, world, sizeof(EcsEvent*), NULL);

    if(manager->events[world] == NULL)
        manager->events[world] = ecs_event_init();

    return ecs_event_add(manager->events[world], function);
}

EcsResult ecs_event_unsubscribe(EcsWorld world, EcsEventManager* manager, int id) {
    if(world >= manager->capacity)
        return ECS_RESULT_INVALID_WORLD;

    EcsEvent* event = manager->events[world];
    if(event == NULL)
        return ECS_RESULT_INVALID_WORLD;

    ecs_event_remove(event, id);
    return ECS_RESULT_SUCCESS;
}

EcsEvent* ecs_event_init(void) {
    EcsEvent* event = ecs_malloc(sizeof(EcsEvent));
    ecs_dispenser_init(&event->dispenser);
    event->subscriptions = NULL;
    event->capacity = 0;
    return event;
}

void ecs_event_free(EcsEvent* event) {
    if(event->subscriptions != NULL)
        ecs_free(event->subscriptions);
    ecs_dispenser_free_resources(&event->dispenser);
    ecs_free(event);
}

int ecs_event_add(EcsEvent* event, void* function) {
    int id = ecs_dispenser_get(&event->dispenser);
    ECS_ARRAY_RESIZE_DEFAULT(event->subscriptions, event->capacity, id, sizeof(void*), NULL);
    event->subscriptions[id] = function;
    return id;
}

void ecs_event_remove(EcsEvent* event, int id) {
    if(id >= event->capacity)
        return;

    ecs_dispenser_release(&event->dispenser, id);
    event->subscriptions[id] = NULL;
}