#include "ecs.h"
#include "ecs_init.h"

ECS_EXPORT void ecs_init(void) {
    ecs_messages_init();
    ecs_world_system_init();
}