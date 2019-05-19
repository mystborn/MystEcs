#include "ecs_entity_set.h"

struct EntitySetBuilder {
    ComponentManager** with_components;
    ComponentManager** without_components;
    ComponentEnum with;
    ComponentEnum without;
    int with_count;
    int with_capacity;
    int without_count;
    int without_capacity;
};

struct EntitySet {
    ComponentManager** with_components;
    ComponentManager** without_components;
    int* with_subscriptions;
    int* without_subscriptions;
    int* mapping;
    Entity* entities;
    ComponentEnum with;
    ComponentEnum without;
    int with_count;
    int without_count;
    int mapping_capacity;
    int entity_capacity;
    int last_index;
    int entity_disposed_subscription;
    int entity_disabled_subscription;
    int entity_enabled_subscription;
    EcsWorld world;
};

struct EntitySetManager {
    IntDispenser dispenser;
    EntitySet* sets;
    int capacity;
};

static EntitySetManager set_manager;

EntitySetBuilder* ecs_entity_set_builder_init(void) {
    EntitySetBuilder* builder = ecs_malloc(sizeof(EntitySetBuilder));
    builder->with_components = NULL;
    builder->without_components = NULL;
    builder->with = COMPONENT_ENUM_DEFAULT;
    builder->without = COMPONENT_ENUM_DEFAULT;
    builder->with_count = 0;
    builder->with_capacity = 0;
    builder->without_count = 0;
    builder->without_capacity = 0;
    return builder;
}

void ecs_entity_set_builder_free(EntitySetBuilder* builder) {
    if(builder->with_components != NULL)
        ecs_free(builder->with_components);

    if(builder->without_components != NULL)
        ecs_free(builder->without_components);

    ecs_component_enum_free_resources(&builder->with);
    ecs_component_enum_free_resources(&builder->without);

    ecs_free(builder);
}

void ecs_entity_set_with(EntitySetBuilder* builder, ComponentManager* manager) {
    ECS_ARRAY_RESIZE(builder->with_components, builder->with_capacity, builder->with_count, sizeof(ComponentManager*));
    builder->with_components[builder->with_count++] = manager;
    ecs_component_enum_set_flag(&builder->with, manager->flag, true);
}

void ecs_entity_set_without(EntitySetBuilder* builder, ComponentManager* manager) {
    ECS_ARRAY_RESIZE(builder->without_components, builder->without_capactiy, builder->without_count, sizeof(ComponentManager*));
    builder->without_components[builder->without_count++] = manager;
    ecs_component_enum_set_flag(&builder->without, manager->flag, true);
}

EntitySet* ecs_entity_set_build(EntitySetBuilder* builder, EcsWorld world, bool free_builder) {
    int id = ecs_dispenser_get(&set_manager.dispenser);
    ECS_ARRAY_RESIZE(set_manager.sets, set_manager.capacity, id, sizeof(EntitySet));
    EntitySet set = set_manager.sets[id];

    set.with_count = builder->with_count;
    set.without_count = builder->without_count;

    set.mapping = NULL;
    set.mapping_capacity = 0;
    set.entities = NULL;
    set.entity_capacity = 0;
    set.last_index = -1;
    set.world = world;

    if(free_builder) {
        set.with_components = builder->with_components;
        set.without_components = builder->without_components;
        set.with = builder->with;
        set.without = builder->without;

        ecs_free(builder);
    } else {
        set.with_components = malloc(sizeof(ComponentManager) * builder->with_count);
        set.without_components = malloc(sizeof(ComponentManager*) * builder->without_count);
        ecs_memcpy(set.with_components, builder->with_components, sizeof(ComponentManager*) * builder->with_count);
        ecs_memcpy(set.without_components, builder->without_components, sizeof(ComponentManager*) * builder->without_count);
        set.with = ecs_component_enum_copy(&builder->with);
        set.without = ecs_component_enum_copy(&builder->without);
    }

    set.with_subscriptions = malloc(sizeof(int) * builder->with_count);
    set.without_subscriptions = malloc(sizeof(int) * builder->without_count);

    return &set;
}