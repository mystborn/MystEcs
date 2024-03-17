#include "ecs_component.h"

#include <stdio.h>

#include "ecs_int_dispenser.h"
#include "ecs_event.h"
#include "ecs_messages.h"
#include "ecs_world.h"

// Creates a link between two entities to the same component.
typedef struct ComponentLink {
    int entity_id;
    int references;
} ComponentLink;

static ComponentLink DEFAULT_COMPONENT_LINK = {0};

// Manages a specific type of component in a world.
typedef struct EcsComponentPool {
    int world;
    int component_size;
    char* components;
    int component_count;
    int* mapping;
    int mapping_count;
    ComponentLink* links;
    int link_count;
    int last_component_index;
    int entity_disposed_id;
} EcsComponentPool;

/**
 * Frees a previously create EcsComponentPool, calling the destructor on each active component if defined.
 * 
 * @param pool The component pool to free.
 * @param destructor The destructor to run for each of the components in the pool. Can be NULL.
 * @param flag The flag of the component that is being freed.
 *             If the world is being destroyed, this should be set to COMPONENT_FLAG_INVALID_MASK
 *             As all of the components will be cleared anyways.
 */
static void ecs_component_pool_free(EcsComponentPool* pool, EcsComponentDestructor destructor, ComponentFlag flag) {
    if(pool->components != NULL) {
        if(pool->mapping != NULL) {
            if(destructor != NULL) {
                for(int i = 0; i <= pool->last_component_index; ++i) {
                    // Here we use the links because it is the only way to make sure there are no double frees in O(n)
                    if(pool->links[i].references != 0) {
                        destructor(pool->components + (pool->mapping[pool->links[i].entity_id] * pool->component_size));
                    }
                }
            }

            if(flag != COMPONENT_FLAG_INVALID_MASK) {
                int throwaway;
                ComponentEnum* components = ecs_world_get_components(pool->world, &throwaway);
                for(int i = 0; i < pool->mapping_count; ++i) {
                    if(pool->mapping[i] == -1)
                        continue;
                    ecs_component_enum_set_flag(components + i, flag, false);
                }
            }
        }

        ecs_free(pool->components);
    }
    
    ecs_free(pool->mapping);
    ecs_free(pool->links);

    ecs_event_unsubscribe(pool->world, ecs_entity_disposed, pool->entity_disposed_id);

    ecs_free(pool);
}

static void component_on_world_disposed(void* data, EcsWorldDisposedMessage* message) {
    EcsComponentManager* manager = data;
    if(message->world < manager->pool_count && manager->pools[message->world] != NULL) {
        ecs_component_pool_free(manager->pools[message->world], manager->destructor, COMPONENT_FLAG_INVALID_MASK);
        manager->pools[message->world] = NULL;
    }
}

static void component_on_entity_disposed(void* data, EcsEntityDisposedMessage* message) {
    EcsComponentManager* manager = data;
    ecs_entity_remove(message->entity, manager);
}

// Initializes a new EcsComponentPool.
static EcsComponentPool* ecs_component_pool_init(int world, int component_size) {
    EcsComponentPool* pool = ecs_malloc(sizeof(EcsComponentPool));
    pool->world = world;
    pool->component_size = component_size;
    pool->components = NULL;
    pool->component_count = 0;
    pool->mapping = NULL;
    pool->mapping_count = 0;
    pool->links = NULL;
    pool->link_count = 0;
    pool->last_component_index = -1;
    return pool;
}

ECS_EXPORT EcsComponentManager* ecs_component_define(int component_size, EcsComponentConstructor constructor, EcsComponentDestructor destructor) {
    EcsComponentManager* manager = ecs_malloc(sizeof(EcsComponentManager));
    manager->flag = ecs_component_flag_get();
    manager->constructor = constructor;
    manager->destructor = destructor;
    manager->added = NULL;
    manager->removed = NULL;
    manager->pools = NULL;
    manager->pool_count = 0;
    manager->component_size = component_size;
    manager->world_disposed_id = ecs_event_add(ecs_world_disposed, ecs_closure(manager, component_on_world_disposed));

    return manager;
}

ECS_EXPORT void ecs_component_free(EcsComponentManager* manager) {
    if(manager->pools != NULL) {
        for(int i = 0; i < manager->pool_count; i++) {
            if(manager->pools[i] != NULL) {
                ecs_component_pool_free(manager->pools[i], manager->destructor, manager->flag);
            }
        }

        ecs_free(manager->pools);
    }

    if(manager->added != NULL)
        ecs_event_manager_free(manager->added);

    if(manager->removed != NULL)
        ecs_event_manager_free(manager->removed);

    ecs_event_remove(ecs_world_disposed, manager->world_disposed_id);

    ecs_free(manager);
}

// Gets or creates the EcsComponentPool for a specific component type on the specified world.
static EcsComponentPool* ecs_component_pool_get_or_create(EcsComponentManager* manager, int world) {
    if(world >= manager->pool_count || manager->pools[world] == NULL) {

        ECS_ARRAY_RESIZE_DEFAULT(manager->pools, manager->pool_count, world, sizeof(*manager->pools), NULL);

        EcsComponentPool* result = ecs_component_pool_init(world, manager->component_size);
        result->entity_disposed_id = ecs_event_subscribe(world, ecs_entity_disposed, ecs_closure(manager, component_on_entity_disposed));

        manager->pools[world] = result;

        return result;
    } else {
        return manager->pools[world];
    }
}

#define ECS_COMPONENT_ADDED(entity, components, manager, result) \
    if(manager->added != NULL && ecs_component_enum_get_flag(components, ecs_is_enabled_flag)) { \
        ecs_event_publish(entity.world,  \
                          manager->added,  \
                          void (*)(void*, EcsComponentAddedMessage*), \
                          &(EcsComponentAddedMessage){ entity, manager, result }); \
    }

#define ECS_COMPONENT_REMOVED(entity, components, manager, result) \
    if (manager->removed != NULL && ecs_component_enum_get_flag(components, ecs_is_enabled_flag)) {\
        ecs_event_publish( \
            entity.world, \
            manager->removed, \
            void (*)(void*, EcsComponentRemovedMessage*), \
            &(EcsComponentRemovedMessage){ entity, manager, result }); \
    }

static void ecs_entity_set_component_memory(void* dest, void* src, EcsComponentManager* component_type) {
    if (src) {
        memcpy(dest, src, component_type->component_size);
    } else {
        memset(dest, 0, component_type->component_size);
    }

    if (component_type->constructor != NULL) {
        component_type->constructor(dest);
    }
}

ECS_EXPORT void* ecs_entity_set(EcsEntity entity, EcsComponentManager* manager, void* value) {
    ComponentEnum* components;
    void* result;
    EcsComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    ECS_ARRAY_RESIZE_DEFAULT(pool->mapping, pool->mapping_count, entity.id, sizeof(*pool->mapping), -1);

    int* index = pool->mapping + entity.id;
    if(*index != -1) {
        result = pool->components + (*index * pool->component_size);

        components = ecs_entity_get_components(entity);
        ECS_COMPONENT_REMOVED(entity, components, manager, result);

        if(manager->destructor != NULL)
            manager->destructor(result);

        ecs_entity_set_component_memory(result, value, manager);

        ECS_COMPONENT_ADDED(entity, components, manager, result);

        return result;
    }

    *index = ++pool->last_component_index;

    ECS_ARRAY_RESIZE(pool->components, pool->component_count, pool->last_component_index, pool->component_size);

    ECS_ARRAY_RESIZE_DEFAULT(pool->links, pool->link_count, pool->last_component_index, sizeof(*pool->links), DEFAULT_COMPONENT_LINK);

    pool->links[pool->last_component_index].entity_id = entity.id;
    pool->links[pool->last_component_index].references = 1;

    result = pool->components + pool->component_size * pool->last_component_index;

    components = ecs_entity_get_components(entity);
    ecs_component_enum_set_flag(components, manager->flag, true);

    ecs_entity_set_component_memory(result, value, manager);

    ECS_COMPONENT_ADDED(entity, components, manager, result);

    return result;

}

ECS_EXPORT EcsResult ecs_entity_set_same_as(EcsEntity entity, EcsEntity reference, EcsComponentManager* manager) {
    if(entity.world != reference.world)
        return ECS_RESULT_DIFFERENT_WORLD;

    if(!ecs_entity_has(reference, manager))
        return ECS_RESULT_INVALID_ENTITY;

    EcsComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    ECS_ARRAY_RESIZE_DEFAULT(pool->mapping, pool->mapping_count, entity.id, sizeof(*pool->mapping), -1);

    int ref_index = pool->mapping[reference.id];
    int* index = pool->mapping + entity.id;

    if(*index != -1) {
        if(*index == ref_index)
            return ECS_RESULT_SUCCESS;

        ecs_entity_remove(entity, manager);
    }

    ++pool->links[ref_index].references;
    *index = ref_index;

    ComponentEnum* components = ecs_entity_get_components(entity);
    ecs_component_enum_set_flag(components, manager->flag, true);
    ECS_COMPONENT_ADDED(entity, components, manager, &pool->components[pool->component_size * ref_index]);

    return ECS_RESULT_SUCCESS;
}

ECS_EXPORT EcsResult ecs_entity_remove(EcsEntity entity, EcsComponentManager* manager) {
    EcsComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return ECS_RESULT_INVALID_ENTITY;

    int* index = pool->mapping + entity.id;
    if(*index == -1)
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = ecs_entity_get_components(entity);
    ecs_component_enum_set_flag(components, manager->flag, false);
    void* component = pool->components + pool->component_size * *index;
    ECS_COMPONENT_REMOVED(entity, components, manager, component);

    ComponentLink* link = pool->links + *index;
    if(--link->references == 0) {
        if(manager->destructor != NULL)
            manager->destructor(component);

        ComponentLink last_link = pool->links[pool->last_component_index];
        pool->links[*index] = last_link;
        ecs_memmove(pool->components + (pool->component_size * *index), pool->components + (pool->component_size * pool->last_component_index), pool->component_size);
        if(last_link.references == 1) {
            pool->mapping[last_link.entity_id] = *index;
        } else {
            for(int i = 0; i < pool->mapping_count; ++i) {
                if(pool->mapping[i] == pool->last_component_index)
                    pool->mapping[i] = *index;
            }
        }

        --pool->last_component_index;
    } else if(link->entity_id == entity.id) {
        int link_index = *index;
        for(int i = 0; i < pool->mapping_count; ++i) {
            if(pool->mapping[i] == link_index && i != entity.id) {
                link->entity_id = i;
                break;
            }
        }
    }

    *index = -1;

    return ECS_RESULT_SUCCESS;
}

ECS_EXPORT EcsResult ecs_entity_get(EcsEntity entity, EcsComponentManager* manager, void** data) {
    EcsComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return ECS_RESULT_INVALID_ENTITY;

    int index = pool->mapping[entity.id];
    if(index == -1)
        return ECS_RESULT_INVALID_ENTITY;

    *data = pool->components + (index * pool->component_size);
    return ECS_RESULT_SUCCESS;
}

ECS_EXPORT bool ecs_entity_has(EcsEntity entity, EcsComponentManager* manager) {
    EcsComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return false;

    return pool->mapping[entity.id] != -1;
}

ECS_EXPORT void* ecs_component_get_all(EcsWorld world, EcsComponentManager* manager, int* count) {
    EcsComponentPool* pool = ecs_component_pool_get_or_create(manager, world);
    *count = pool->last_component_index + 1;
    return pool->components;
}