#include "entity_set.h"
#include "ecs_messages.h"

#include <stdio.h>

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
    int entity_created_subscription;
    EcsWorld world;
};

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
    ECS_ARRAY_RESIZE(builder->without_components, builder->without_capacity, builder->without_count, sizeof(ComponentManager*));
    builder->without_components[builder->without_count++] = manager;
    ecs_component_enum_set_flag(&builder->without, manager->flag, true);
}

static void entity_set_add(EntitySet* set, Entity entity) {
    ECS_ARRAY_RESIZE_DEFAULT(set->mapping, set->mapping_capacity, entity.id, sizeof(int), -1);

    int* index = set->mapping + entity.id;
    if(*index == -1) {
        *index = ++set->last_index;

        ECS_ARRAY_RESIZE(set->entities, set->entity_capacity, *index, sizeof(Entity));

        set->entities[*index] = entity;
    }
}

static void entity_set_remove(EntitySet* set, Entity entity) {
    if(entity.id >= set->mapping_capacity)
        return;

    int* index = set->mapping + entity.id;
    if(*index == -1)
        return;

    if(*index != set->last_index) {
        set->entities[*index] = set->entities[set->last_index];
        set->mapping[set->entities[set->last_index].id] = *index;
    }

    --set->last_index;
    *index = -1;
}

static inline bool entity_set_filter_enum(EntitySet* set, ComponentEnum* cenum) {
    return ecs_component_enum_contains_enum(cenum, &set->with) && ecs_component_enum_not_contains_enum(cenum, &set->without);
}

static void entity_set_entity_created_add(void* data, EcsEntityCreatedMessage* message) {
    entity_set_add((EntitySet*)data, message->entity);
}

static void entity_set_entity_enabled_checked_add(void* data, EcsEntityEnabledMessage* message) {
    EntitySet* set = data;
    if(entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_add(set, message->entity);
}

static void entity_set_component_added_checked_add(void* data, EcsComponentAddedMessage* message) {
    EntitySet* set = data;
    if(entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_add(set, message->entity);
}

static void entity_set_component_removed_checked_add(void* data, EcsComponentRemovedMessage* message) {
    EntitySet* set = data;
    if(entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_add(set, message->entity);
}

static void entity_set_entity_disposed_remove(void* data, EcsEntityDisposedMessage* message) {
    entity_set_remove((EntitySet*)data, message->entity);
}

static void entity_set_entity_disabled_remove(void* data, EcsEntityDisabledMessage* message) {
    entity_set_remove((EntitySet*)data, message->entity);
}

static void entity_set_component_added_remove(void* data, EcsComponentAddedMessage* message) {
    entity_set_remove((EntitySet*)data, message->entity);
}

static void entity_set_component_removed_remove(void* data, EcsComponentRemovedMessage* message) {
    entity_set_remove((EntitySet*)data, message->entity);
}

EntitySet* ecs_entity_set_build(EntitySetBuilder* builder, EcsWorld world, bool free_builder) {
    EntitySet* set = ecs_malloc(sizeof(EntitySet));

    set->with_count = builder->with_count;
    set->without_count = builder->without_count;

    set->mapping = NULL;
    set->mapping_capacity = 0;
    set->entities = NULL;
    set->entity_capacity = 0;
    set->last_index = -1;
    set->world = world;

    if(free_builder) {
        set->with_components = builder->with_components;
        set->without_components = builder->without_components;
        set->with = builder->with;
        set->without = builder->without;

        ecs_free(builder);
    } else {
        set->with_components = malloc(sizeof(ComponentManager*) * builder->with_count);
        set->without_components = malloc(sizeof(ComponentManager*) * builder->without_count);
        ecs_memcpy(set->with_components, builder->with_components, sizeof(ComponentManager*) * builder->with_count);
        ecs_memcpy(set->without_components, builder->without_components, sizeof(ComponentManager*) * builder->without_count);
        set->with = ecs_component_enum_copy(&builder->with);
        set->without = ecs_component_enum_copy(&builder->without);
    }

    ecs_component_enum_set_flag(&set->with, is_alive_flag, true);
    ecs_component_enum_set_flag(&set->with, is_enabled_flag, true);

    set->entity_disposed_subscription = ecs_event_subscribe(world,
                                                            ecs_entity_disposed,
                                                            ecs_closure(set, entity_set_entity_disposed_remove));

    set->entity_disabled_subscription = ecs_event_subscribe(world,
                                                            ecs_entity_disabled,
                                                            ecs_closure(set, entity_set_entity_disabled_remove));

    set->entity_enabled_subscription = ecs_event_subscribe(world,
                                                           ecs_entity_enabled,
                                                           ecs_closure(set, entity_set_entity_enabled_checked_add));

    if(set->with_count == 0) {
        set->entity_created_subscription = ecs_event_subscribe(world,
                                                               ecs_entity_created,
                                                               ecs_closure(set, entity_set_entity_created_add));
    }

    set->with_subscriptions = malloc(sizeof(int) * set->with_count * 2);
    set->without_subscriptions = malloc(sizeof(int) * set->without_count * 2);

    for(int i = 0; i < set->with_count; i++) {
        set->with_subscriptions[i * 2] = ecs_event_subscribe(world,
                                                             ecs_component_get_added_event(set->with_components[i]), 
                                                             ecs_closure(set, entity_set_component_added_checked_add));

        set->with_subscriptions[i * 2 + 1] = ecs_event_subscribe(world,
                                                                 ecs_component_get_removed_event(set->with_components[i]),
                                                                 ecs_closure(set, entity_set_component_removed_remove));
    }

    for(int i = 0; i < set->without_count; i++) {
        set->without_subscriptions[i * 2] = ecs_event_subscribe(world,
                                                                ecs_component_get_removed_event(set->without_components[i]),
                                                                ecs_closure(set, entity_set_component_removed_checked_add));

        set->without_subscriptions[i * 2 + 1] = ecs_event_subscribe(world,
                                                                    ecs_component_get_added_event(set->without_components[i]),
                                                                    ecs_closure(set, entity_set_component_added_remove));
    }

    // Fill the set with existing components that match the component conditions.
    int entity_count;
    ComponentEnum* components = ecs_world_get_components(world, &entity_count);

    for(int i = 0; i < entity_count; i++, components++) {
        if(entity_set_filter_enum(set, components))
            entity_set_add(set, (Entity){ .world = world, .id = i });
    }

    return set;
}

void ecs_entity_set_free(EntitySet* set) {
    for(int i = 0; i < set->with_count; i++) {
        ecs_event_unsubscribe(set->world, 
                              ecs_component_get_added_event(set->with_components[i]), 
                              set->with_subscriptions[i * 2]);

        ecs_event_unsubscribe(set->world,
                              ecs_component_get_removed_event(set->with_components[i]),
                              set->with_subscriptions[i * 2 + 1]);
    }

    for(int i = 0; i < set->without_count; i++) {
        ecs_event_unsubscribe(set->world,
                              ecs_component_get_removed_event(set->without_components[i]),
                              set->without_subscriptions[i * 2]);

        ecs_event_unsubscribe(set->world,
                              ecs_component_get_added_event(set->without_components[i]),
                              set->without_subscriptions[i * 2 + 1]);
    }

    ecs_free(set->with_subscriptions);
    ecs_free(set->without_subscriptions);

    if(set->with_count == 0)
        ecs_event_unsubscribe(set->world,  ecs_entity_created, set->entity_created_subscription);

    ecs_event_unsubscribe(set->world, ecs_entity_enabled, set->entity_enabled_subscription);
    ecs_event_unsubscribe(set->world, ecs_entity_disabled, set->entity_disabled_subscription);
    ecs_event_unsubscribe(set->world, ecs_entity_disposed, set->entity_disposed_subscription);

    ecs_free(set->with_components);
    ecs_free(set->without_components);

    ecs_component_enum_free_resources(&set->with);
    ecs_component_enum_free_resources(&set->without);

    ecs_free(set->mapping);
    ecs_free(set->entities);

    ecs_free(set);
}

Entity* ecs_entity_set_get_entities(EntitySet* set, int* count) {
    *count = set->last_index + 1;
    return set->entities;
}