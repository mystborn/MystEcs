#include "ecs_messages.h"
#include "ecs_init.h"

ECS_EXPORT EcsEventManager* ecs_entity_created;
ECS_EXPORT EcsEventManager* ecs_entity_disposed;
ECS_EXPORT EcsEventManager* ecs_entity_enabled;
ECS_EXPORT EcsEventManager* ecs_entity_disabled;
ECS_EXPORT EcsEvent* ecs_world_disposed;

void ecs_messages_init(void) {
    // This has to be initialized first, because EcsEventManagers add a function to it.
    ecs_world_disposed = ecs_event_init();

    ecs_entity_created = ecs_event_define();
    ecs_entity_disposed = ecs_event_define();
    ecs_entity_enabled = ecs_event_define();
    ecs_entity_disabled = ecs_event_define();
}