#include "ecs_event.h"
#include "ecs_array.h"
#include "ecs_messages.h"

static EcsClosure ECS_CLOSURE_DEFAULT = { NULL, NULL, false };

static void event_on_world_disposed(void* data, EcsWorldDisposedMessage* message) {
    EcsEventManager* manager = data;
    if((unsigned int)message->world < manager->capacity && manager->events[message->world] != NULL) {
        ecs_event_free(manager->events[message->world]);
        manager->events[message->world] = NULL;
    }
}

EcsEventManager* ecs_event_define(void) {
    EcsEventManager* manager = ecs_malloc(sizeof(EcsEventManager));

    manager->events = NULL;
    manager->capacity = 0;
    manager->id = ecs_event_add(ecs_world_disposed, ecs_closure(manager, event_on_world_disposed));

    return manager;
}

void ecs_event_manager_free(EcsEventManager* manager) {
    ecs_event_remove(ecs_world_disposed, manager->id);

    if(manager->events != NULL) {
        for(int i = 0; i < manager->capacity; ++i) {
            if(manager->events[i] != NULL)
                ecs_event_free(manager->events[i]);
        }
        ecs_free(manager->events);
    }

    ecs_free(manager);
}

int ecs_event_subscribe(EcsWorld world, EcsEventManager* manager, EcsClosure closure) {
    ECS_ARRAY_RESIZE_DEFAULT(manager->events, manager->capacity, world, sizeof(EcsEvent*), NULL);

    if(manager->events[world] == NULL)
        manager->events[world] = ecs_event_init();

    return ecs_event_add(manager->events[world], closure);
}

EcsResult ecs_event_unsubscribe(EcsWorld world, EcsEventManager* manager, int id) {
    if(world >= manager->capacity)
        return ECS_RESULT_INVALID_WORLD;

    EcsEvent* event = manager->events[world];
    if(event == NULL)
        return ECS_RESULT_INVALID_WORLD;

    return ecs_event_remove(event, id) ? ECS_RESULT_SUCCESS : ECS_RESULT_INVALID_WORLD;
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

int ecs_event_add(EcsEvent* event, EcsClosure closure) {
    int id = ecs_dispenser_get(&event->dispenser);
    ECS_ARRAY_RESIZE_DEFAULT(event->subscriptions, event->capacity, id, sizeof(EcsClosure), ECS_CLOSURE_DEFAULT);
    event->subscriptions[id] = closure;
    event->subscriptions[id].active = true;

    return id;
}

bool ecs_event_remove(EcsEvent* event, int id) {
    if((unsigned int)id >= event->capacity || !event->subscriptions[id].active)
        return false;

    event->subscriptions[id].active = false;

    ecs_dispenser_release(&event->dispenser, id);
    return true;
}