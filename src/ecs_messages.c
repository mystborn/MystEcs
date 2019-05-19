#include "ecs_messages.h"

EcsEventManager* ecs_entity_created;
EcsEventManager* ecs_entity_disposed;
EcsEventManager* ecs_entity_enabled;
EcsEventManager* ecs_entity_disabled;

void ecs_messages_init(void) {
    ecs_entity_created = ecs_event_define();
    ecs_entity_disposed = ecs_event_define();
    ecs_entity_enabled = ecs_event_define();
    ecs_entity_disabled = ecs_event_define();

#if ECS_DEBUG
    puts("Messages Initialized");
#endif
}