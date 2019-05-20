#include "ecs_messages.h"

EcsEventManager* ecs_entity_created;
EcsEventManager* ecs_entity_disposed;
EcsEventManager* ecs_entity_enabled;
EcsEventManager* ecs_entity_disabled;
EcsEvent* ecs_world_disposed;

void ecs_messages_init(void) {
    // This has to be initialized first, because EcsEventManagers add a function to it.
    ecs_world_disposed = ecs_event_init();

    ecs_entity_created = ecs_event_define();
    ecs_entity_disposed = ecs_event_define();
    ecs_entity_enabled = ecs_event_define();
    ecs_entity_disabled = ecs_event_define();
}