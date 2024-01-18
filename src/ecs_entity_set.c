#include "ecs_entity_set.h"
#include "ecs_messages.h"

#include <stdio.h>

typedef struct EcsEitherComponent {
    EcsComponentManager** components;
    int components_count;
} EcsEitherComponent;

struct EcsEntitySetBuilder {
    EcsComponentManager** with_components;
    EcsComponentManager** without_components;
    EcsEitherComponent* with_any_components;
    ComponentEnum with;
    ComponentEnum without;
    int with_count;
    int with_capacity;
    int with_any_count;
    int without_count;
    int without_capacity;
    int with_any_capacity;
};

struct EcsEntitySet {
    EcsComponentManager** with_components;
    EcsComponentManager** without_components;
    EcsEitherComponent* with_any_components;
    int* with_subscriptions;
    int* without_subscriptions;
    int* mapping;
    EcsEntity* entities;
    ComponentEnum with;
    ComponentEnum without;
    int with_count;
    int without_count;
    int with_any_count;
    int mapping_capacity;
    int entity_capacity;
    int last_index;
    int entity_disabled_subscription;
    int entity_enabled_subscription;
    int entity_created_subscription;
    EcsWorld world;
};

EcsEntitySetBuilder* ecs_entity_set_builder_init(void) {
    EcsEntitySetBuilder* builder = ecs_malloc(sizeof(EcsEntitySetBuilder));
    builder->with_components = NULL;
    builder->without_components = NULL;
    builder->with_any_components = NULL;
    builder->with = COMPONENT_ENUM_DEFAULT;
    builder->without = COMPONENT_ENUM_DEFAULT;
    builder->with_count = 0;
    builder->with_capacity = 0;
    builder->without_count = 0;
    builder->without_capacity = 0;
    builder->with_any_count = 0;
    builder->with_any_capacity = 0;
    return builder;
}

void ecs_entity_set_builder_free(EcsEntitySetBuilder* builder) {
    ecs_free(builder->with_components);
    ecs_free(builder->without_components);

    ecs_component_enum_free_resources(&builder->with);
    ecs_component_enum_free_resources(&builder->without);

    for (int i = 0; i < builder->with_any_count; i++) {
        EcsEitherComponent* either = builder->with_any_components + i;
        ecs_free(either->components);
    }

    ecs_free(builder->with_any_components);

    ecs_free(builder);
}

void ecs_entity_set_with(EcsEntitySetBuilder* builder, EcsComponentManager* manager) {
    ECS_ARRAY_RESIZE(
        builder->with_components,
        builder->with_capacity,
        builder->with_count + 1,
        sizeof(EcsComponentManager*));

    builder->with_components[builder->with_count++] = manager;
    ecs_component_enum_set_flag(&builder->with, manager->flag, true);
}

void ecs_entity_set_without(EcsEntitySetBuilder* builder, EcsComponentManager* manager) {
    ECS_ARRAY_RESIZE(
        builder->without_components,
        builder->without_capacity,
        builder->without_count + 1,
        sizeof(EcsComponentManager*));

    builder->without_components[builder->without_count++] = manager;
    ecs_component_enum_set_flag(&builder->without, manager->flag, true);
}

void ecs_entity_set_with_any(EcsEntitySetBuilder* builder, EcsComponentManager** components, int count) {
    ECS_ARRAY_RESIZE(
        builder->with_any_components,
        builder->with_any_capacity,
        builder->with_any_count + 1,
        sizeof(EcsEitherComponent));

    EcsEitherComponent* either = builder->with_any_components + builder->with_any_count++;
    either->components = ecs_malloc(sizeof(EcsComponentManager*) * count);
    either->components_count = count;
    ecs_memcpy(either->components, components, sizeof(EcsComponentManager*) * count);
}

static void entity_set_add(EcsEntitySet* set, EcsEntity entity) {
    ECS_ARRAY_RESIZE_DEFAULT(set->mapping, set->mapping_capacity, entity.id, sizeof(int), -1);

    int* index = set->mapping + entity.id;
    if(*index == -1) {
        *index = ++set->last_index;

        ECS_ARRAY_RESIZE(set->entities, set->entity_capacity, *index, sizeof(EcsEntity));

        set->entities[*index] = entity;
    }
}

static void entity_set_remove(EcsEntitySet* set, EcsEntity entity) {
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

static inline bool entity_set_filter_enum(EcsEntitySet* set, ComponentEnum* cenum) {
    // Loop through the with_any sets first.
    // For each set, determine if the ComponentEnum has at least one of the components in the set.
    // If a component is found, immediately move on to the next set.
    for (int i = 0; i < set->with_any_count; i++) {
        EcsEitherComponent* either = set->with_any_components + i;
        bool has = false;
        for (int j = 0; j < either->components_count; j++) {
            if (ecs_component_enum_get_flag(cenum, either->components[j]->flag)) {
                has = true;
                break;
            }
        }
        if (!has)
            return false;
    }

    // If all with_any conditions are true, make sure the ComponentEnum also has all of the
    // with components and none of the without components.
    return ecs_component_enum_contains_enum(cenum, &set->with) 
        && ecs_component_enum_not_contains_enum(cenum, &set->without);
}

static void entity_set_entity_created_add(void* data, EcsEntityCreatedMessage* message) {
    entity_set_add((EcsEntitySet*)data, message->entity);
}

static void entity_set_entity_enabled_checked_add(void* data, EcsEntityEnabledMessage* message) {
    EcsEntitySet* set = data;
    if(entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_add(set, message->entity);
}

static void entity_set_component_added_checked_add(void* data, EcsComponentAddedMessage* message) {
    EcsEntitySet* set = data;
    if(entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_add(set, message->entity);
}

static void entity_set_component_removed_checked_add(void* data, EcsComponentRemovedMessage* message) {
    EcsEntitySet* set = data;
    if(entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_add(set, message->entity);
}

static void entity_set_component_removed_checked_remove(void* data, EcsComponentRemovedMessage* message) {
    EcsEntitySet* set = data;
    if(!entity_set_filter_enum(set, ecs_entity_get_components(message->entity)))
        entity_set_remove(set, message->entity);
}

static void entity_set_entity_disposed_remove(void* data, EcsEntityDisposedMessage* message) {
    entity_set_remove((EcsEntitySet*)data, message->entity);
}

static void entity_set_entity_disabled_remove(void* data, EcsEntityDisabledMessage* message) {
    entity_set_remove((EcsEntitySet*)data, message->entity);
}

static void entity_set_component_added_remove(void* data, EcsComponentAddedMessage* message) {
    entity_set_remove((EcsEntitySet*)data, message->entity);
}

static void entity_set_component_removed_remove(void* data, EcsComponentRemovedMessage* message) {
    entity_set_remove((EcsEntitySet*)data, message->entity);
}

EcsEntitySet* ecs_entity_set_build(EcsEntitySetBuilder* builder, EcsWorld world, bool free_builder) {
    EcsEntitySet* set = ecs_malloc(sizeof(EcsEntitySet));

    set->with_count = builder->with_count;
    set->without_count = builder->without_count;
    set->with_any_count = builder->with_any_count;

    set->mapping = NULL;
    set->mapping_capacity = 0;
    set->entities = NULL;
    set->entity_capacity = 0;
    set->last_index = -1;
    set->world = world;

    // If there are no intentions to use the builder any further, then we can just steal
    // the data from the builder for the set.
    // Otherwise, we'll need to copy over all of the dynamic data.
    if(free_builder) {
        set->with_components = builder->with_components;
        set->without_components = builder->without_components;
        set->with_any_components = builder->with_any_components;
        set->with = builder->with;
        set->without = builder->without;

        ecs_free(builder);
    } else {
        set->with_components = malloc(sizeof(EcsComponentManager*) * builder->with_count);
        set->without_components = malloc(sizeof(EcsComponentManager*) * builder->without_count);
        set->with_any_components = malloc(sizeof(EcsEitherComponent) * builder->with_any_count);
        ecs_memcpy(set->with_components, builder->with_components, sizeof(EcsComponentManager*) * builder->with_count);
        ecs_memcpy(set->without_components, builder->without_components, sizeof(EcsComponentManager*) * builder->without_count);
        for (int i = 0; i < builder->with_any_count; i++) {
            EcsEitherComponent* set_either = set->with_any_components + i;
            EcsEitherComponent* builder_either = builder->with_any_components + i;
            set_either->components_count = builder_either->components_count;
            set_either->components = malloc(sizeof(EcsComponentManager*) * set_either->components_count);
            ecs_memcpy(set_either->components, builder_either->components, sizeof(EcsComponentManager*) * set_either->components_count);
        }
        set->with = ecs_component_enum_copy(&builder->with);
        set->without = ecs_component_enum_copy(&builder->without);
    }

    ecs_component_enum_set_flag(&set->with, ecs_is_alive_flag, true);
    ecs_component_enum_set_flag(&set->with, ecs_is_enabled_flag, true);

    set->entity_disabled_subscription = ecs_event_subscribe(
        world,
        ecs_entity_disabled,
        ecs_closure(set, entity_set_entity_disabled_remove));

    set->entity_enabled_subscription = ecs_event_subscribe(
        world,
        ecs_entity_enabled,
        ecs_closure(set, entity_set_entity_enabled_checked_add));

    if(set->with_count == 0 && set->with_any_count == 0) {
        set->entity_created_subscription = ecs_event_subscribe(
            world,
            ecs_entity_created,
            ecs_closure(set, entity_set_entity_created_add));
    }

    int with_any_subscription_count = 0;
    for (int i = 0; i < set->with_any_count; i++) {
        EcsEitherComponent* either = set->with_any_components + i;
        with_any_subscription_count += either->components_count;
    }

    set->with_subscriptions = malloc(sizeof(int) * (set->with_count * 2 + with_any_subscription_count * 2));
    set->without_subscriptions = malloc(sizeof(int) * set->without_count * 2);

    for(int i = 0; i < set->with_count; i++) {
        set->with_subscriptions[i * 2] = ecs_event_subscribe(
            world,
            ecs_component_get_added_event(set->with_components[i]), 
            ecs_closure(set, entity_set_component_added_checked_add));

        set->with_subscriptions[i * 2 + 1] = ecs_event_subscribe(
            world,
            ecs_component_get_removed_event(set->with_components[i]),
            ecs_closure(set, entity_set_component_removed_remove));
    }

    for(int i = 0; i < set->without_count; i++) {
        set->without_subscriptions[i * 2] = ecs_event_subscribe(
            world,
            ecs_component_get_removed_event(set->without_components[i]),
            ecs_closure(set, entity_set_component_removed_checked_add));

        set->without_subscriptions[i * 2 + 1] = ecs_event_subscribe(
            world,
            ecs_component_get_added_event(set->without_components[i]),
            ecs_closure(set, entity_set_component_added_remove));
    }

    int with_index = set->with_count * 2;
    for(int i = 0; i < set->with_any_count; i++) {
        EcsEitherComponent* either = set->with_any_components + i;
        for(int j = 0; j < either->components_count; j++) {
            set->with_subscriptions[with_index++] = ecs_event_subscribe(
                world,
                ecs_component_get_added_event(either->components[j]),
                ecs_closure(set, entity_set_component_added_checked_add));

            set->with_subscriptions[with_index++] = ecs_event_subscribe(
                world,
                ecs_component_get_removed_event(either->components[j]),
                ecs_closure(set, entity_set_component_removed_checked_remove));
        }
    }

    // Fill the set with existing components that match the component conditions.
    int entity_count;
    ComponentEnum* components = ecs_world_get_components(world, &entity_count);

    for(int i = 0; i < entity_count; i++, components++) {
        if(entity_set_filter_enum(set, components))
            entity_set_add(set, (EcsEntity){ .world = world, .id = i });
    }

    return set;
}

void ecs_entity_set_free(EcsEntitySet* set) {
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

    int with_count = set->with_count * 2;
    for (int i = 0; i < set->with_any_count; i++) {
        EcsEitherComponent* either = set->with_any_components + i;
        for (int j = 0; j < either->components_count; j++) {
            ecs_event_unsubscribe(
                set->world,
                ecs_component_get_added_event(either->components[j]),
                set->with_subscriptions[with_count++]);

            ecs_event_unsubscribe(
                set->world,
                ecs_component_get_removed_event(either->components[j]),
                set->with_subscriptions[with_count++]);
        }
        free(either->components);
    }

    ecs_free(set->with_subscriptions);
    ecs_free(set->without_subscriptions);

    if(set->with_count == 0 && set->with_any_count == 0)
        ecs_event_unsubscribe(set->world,  ecs_entity_created, set->entity_created_subscription);

    ecs_event_unsubscribe(set->world, ecs_entity_enabled, set->entity_enabled_subscription);
    ecs_event_unsubscribe(set->world, ecs_entity_disabled, set->entity_disabled_subscription);

    ecs_free(set->with_components);
    ecs_free(set->without_components);
    ecs_free(set->with_any_components);

    ecs_component_enum_free_resources(&set->with);
    ecs_component_enum_free_resources(&set->without);

    ecs_free(set->mapping);
    ecs_free(set->entities);

    ecs_free(set);
}

EcsEntity* ecs_entity_set_get_entities(EcsEntitySet* set, int* count) {
    *count = set->last_index + 1;
    return set->entities;
}