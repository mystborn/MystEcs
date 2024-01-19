#include "ecs.h"
#include "ecs_init.h"

void ecs_init(void) {
    ecs_messages_init();
    ecs_world_system_init();
}