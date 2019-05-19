#include "ecs.h"

#if ECS_DEBUG
#include <stdio.h>
#endif

void ecs_init(void) {
#if ECS_DEBUG
    puts("Ecs Initializing");
#endif
    ecs_event_system_init();
    ecs_messages_init();
    ecs_world_system_init();
    ecs_component_management_system_init();

#if ECS_DEBUG
    puts("Ecs Finished Initializing");
#endif
}