#include "ecs_world.h"

#include <stdio.h>

#include "ecs_messages.h"
#include "ecs_int_dispenser.h"

struct EcsWorldImpl {
    EcsIntDispenser dispenser;
    ComponentEnum* entity_components;
    int capacity;
};

struct EcsWorldManager {
    EcsIntDispenser dispenser;
    struct EcsWorldImpl* worlds;
    int capacity;
};

static struct EcsWorldManager world_manager;

ComponentFlag ecs_is_alive_flag;
ComponentFlag ecs_is_enabled_flag;

void ecs_world_system_init(void) {
    ecs_is_alive_flag = ecs_component_flag_get();
    ecs_is_enabled_flag = ecs_component_flag_get();

    ecs_dispenser_init_start(&world_manager.dispenser, 1);
    world_manager.worlds = NULL;
    world_manager.capacity = 0;
}

EcsWorld ecs_world_init(void) {
    EcsWorld id = ecs_dispenser_get(&world_manager.dispenser);

    ECS_ARRAY_RESIZE(world_manager.worlds, world_manager.capacity, id, sizeof(struct EcsWorldImpl));

    struct EcsWorldImpl* world = world_manager.worlds + id;

    ecs_dispenser_init(&world->dispenser);
    world->entity_components = NULL;
    world->capacity = 0;
    
    return id;
}

EcsResult ecs_world_free(EcsWorld world) {
    if((unsigned int)world >= world_manager.capacity)
        return ECS_RESULT_INVALID_WORLD;

    EcsWorldDisposedMessage message = { world };
    ecs_event_trigger(ecs_world_disposed, void (*)(void*, EcsWorldDisposedMessage*), &message);

    struct EcsWorldImpl* impl = world_manager.worlds + world;

    ecs_dispenser_free_resources(&impl->dispenser);

    if(impl->entity_components != NULL) {
        for(int i = 0; i < impl->capacity; ++i)
            ecs_component_enum_free_resources(impl->entity_components + i);
        
        ecs_free(impl->entity_components);
    }

    ecs_dispenser_release(&world_manager.dispenser, world);

    return ECS_RESULT_SUCCESS;
}

EcsEntity ecs_create_entity(EcsWorld world) {
    struct EcsWorldImpl* impl = world_manager.worlds + world;
    int id = ecs_dispenser_get(&impl->dispenser);
    ECS_ARRAY_RESIZE_DEFAULT(impl->entity_components, impl->capacity, id, sizeof(ComponentEnum), COMPONENT_ENUM_DEFAULT);
    ComponentEnum* entity_components = impl->entity_components + id;

    ecs_component_enum_set_flag(entity_components, ecs_is_alive_flag, true);
    ecs_component_enum_set_flag(entity_components, ecs_is_enabled_flag, true);

    EcsEntity result = { world, id };

    EcsEcsEntityCreatedMessage message = { result };

    ecs_event_publish(world, ecs_entity_created, void (*)(void*, EcsEcsEntityCreatedMessage*), &message);

    return result;
}

EcsResult ecs_entity_free(EcsEntity entity) {
    if((unsigned int)entity.world >= world_manager.capacity)
        return ECS_RESULT_INVALID_WORLD;

    struct EcsWorldImpl* impl = world_manager.worlds + entity.world;
    if((unsigned int)entity.id >= impl->capacity)
        return ECS_RESULT_INVALID_ENTITY;

    EcsEcsEntityDisposedMessage message = { entity };
    ecs_event_publish(entity.world, ecs_entity_disposed, void (*)(void*, EcsEcsEntityDisposedMessage*), &message);
    
    ecs_component_enum_clear(impl->entity_components + entity.id);
    ecs_dispenser_release(&impl->dispenser, entity.id);

    return ECS_RESULT_SUCCESS;
}

EcsResult ecs_entity_enable(EcsEntity entity) {
    if((unsigned int)entity.world >= world_manager.capacity)
        return ECS_RESULT_INVALID_WORLD;

    struct EcsWorldImpl* impl = world_manager.worlds + entity.world;
    if((unsigned int)entity.id >= impl->capacity)
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = impl->entity_components + entity.id;
    if(!ecs_component_enum_get_flag(components, ecs_is_enabled_flag)) {
        ecs_component_enum_set_flag(components, ecs_is_enabled_flag, true);
        EcsEcsEntityEnabledMessage message = { entity };
        ecs_event_publish(entity.world, ecs_entity_enabled, void (*)(void*, EcsEcsEntityEnabledMessage*), &message);

        return ECS_RESULT_SUCCESS;
    }

    return ECS_RESULT_INVALID_STATE;
}

EcsResult ecs_entity_disable(EcsEntity entity) {
    if((unsigned int)entity.world >= world_manager.capacity)
        return ECS_RESULT_INVALID_WORLD;

    struct EcsWorldImpl* impl = world_manager.worlds + entity.world;
    if((unsigned int)entity.id >= impl->capacity)
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = impl->entity_components + entity.id;
    if(ecs_component_enum_get_flag(components, ecs_is_enabled_flag)) {
        ecs_component_enum_set_flag(components, ecs_is_enabled_flag, false);
        EcsEcsEntityDisabledMessage message = { entity };
        ecs_event_publish(entity.world, ecs_entity_disabled, void (*)(void*, EcsEcsEntityDisabledMessage*), &message);

        return ECS_RESULT_SUCCESS;
    }

    return ECS_RESULT_INVALID_STATE;
}

bool ecs_entity_is_alive(EcsEntity entity) {
    if((unsigned int)entity.world >= world_manager.capacity)
        return ECS_RESULT_INVALID_WORLD;

    struct EcsWorldImpl* impl = world_manager.worlds + entity.world;
    if((unsigned int)entity.id >= impl->capacity)
        return false;
    return ecs_component_enum_get_flag(impl->entity_components + entity.id, ecs_is_alive_flag);
}

bool ecs_entity_is_enabled(EcsEntity entity) {
    if((unsigned int)entity.world >= world_manager.capacity)
        return ECS_RESULT_INVALID_WORLD;

    struct EcsWorldImpl* impl = world_manager.worlds + entity.world;
    if((unsigned int)entity.id >= impl->capacity)
        return false;
    return ecs_component_enum_get_flag(impl->entity_components + entity.id, ecs_is_enabled_flag);
}

ComponentEnum* ecs_entity_get_components(EcsEntity entity) {
    struct EcsWorldImpl* impl = world_manager.worlds + entity.world;
    return impl->entity_components + entity.id;
}

ComponentEnum* ecs_world_get_components(EcsWorld world, int* count) {
    struct EcsWorldImpl* impl = world_manager.worlds + world;

    *count = impl->dispenser.total;
    
    return impl->entity_components;
}