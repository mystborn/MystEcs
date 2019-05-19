#include "component.h"

#include <stdio.h>

#include "int_dispenser.h"
#include "ecs_event.h"
#include "ecs_messages.h"
#include "ecs_world.h"

// =========================================================================================================================
// Components
//
// Defines the types and methods used to manage components.

struct ComponentManagementSystem {
    IntDispenser dispenser;
    ComponentManager* managers;
    int capacity;
};

// Creates a link between two entities to the same component.
typedef struct ComponentLink {
    int entity_id;
    int references;
} ComponentLink;

static ComponentLink DEFAULT_COMPONENT_LINK = {0};

// Manages a specific type of component in a world.
typedef struct ComponentPool {
    int world;
    int component_size;
    char* components;
    int component_count;
    int* mapping;
    int mapping_count;
    ComponentLink* links;
    int link_count;
    int last_component_index;
} ComponentPool;

static struct ComponentManagementSystem component_management_system;

// Frees a previously create ComponentPool, calling the destructor on each active component if defined.
// @param flag The flag of the component that is being freed.
//             If the world is being destroyed, this should be set to COMPONENT_FLAG_INVALID_MASK
//             As all of the components will be cleared anyways.
static void ecs_component_pool_free(ComponentPool* pool, ComponentDestructor destructor, ComponentFlag flag) {
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

    if(pool->mapping != NULL)
        ecs_free(pool->mapping);

    if(pool->links != NULL)
        ecs_free(pool->links);

    ecs_free(pool);
}

static void on_world_disposed(EcsWorldDisposedMessage* message) {
    for(int i = 0; i < component_management_system.dispenser.total; ++i) {
        ComponentManager* manager = component_management_system.managers + i;
        if(manager->id != -1 && message->world < manager->pool_count && manager->pools[message->world] != NULL)
        {
            ecs_component_pool_free(manager->pools[message->world], manager->destructor, COMPONENT_FLAG_INVALID_MASK);
            manager->pools[message->world] = NULL;

        }
    }
}

static void on_entity_disposed(EcsEntityDisposedMessage* message) {
    for(int i = 0; i < component_management_system.dispenser.total; ++i) {
        ComponentManager* manager = component_management_system.managers + i;
        if(manager->id != -1)
            ecs_component_remove(message->entity, manager);
    }
}

// Initializes a new ComponentPool.
static ComponentPool* ecs_component_pool_init(int world, int component_size) {
    ComponentPool* pool = ecs_malloc(sizeof(ComponentPool));
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

void ecs_component_management_system_init(void) {
    ecs_dispenser_init(&component_management_system.dispenser);
    component_management_system.managers = NULL;
    component_management_system.capacity = 0;

    ecs_event_add(ecs_world_disposed, on_world_disposed);
    ecs_event_subscribe(0, ecs_entity_disposed, on_entity_disposed);


#if ECS_DEBUG
    puts("Component Management System Initialized");
#endif
}

ComponentManager* ecs_component_define(int component_size, ComponentConstructor constructor, ComponentDestructor destructor) {
    int id = ecs_dispenser_get(&component_management_system.dispenser);
    ECS_ARRAY_RESIZE(component_management_system.managers, component_management_system.capacity, id, sizeof(ComponentManager));
    ComponentManager* manager = component_management_system.managers + id;
    manager->flag = ecs_component_flag_get();
    manager->constructor = constructor;
    manager->destructor = destructor;
    manager->added = NULL;
    manager->removed = NULL;
    manager->pools = NULL;
    manager->pool_count = 0;
    manager->component_size = component_size;
    manager->id = id;

    return manager;
}

void ecs_component_free(ComponentManager* manager) {
    if(manager->pools != NULL) {
        for(int i = 0; i < manager->pool_count; i++) {
            if(manager->pools[i] != NULL) {
                ecs_component_pool_free(manager->pools[i], manager->destructor, manager->flag);
            }
        }

        ecs_free(manager->pools);
    }

    ecs_dispenser_release(&component_management_system.dispenser, manager->id);
    manager->pools = NULL;
    manager->pool_count = 0;
    manager->id = -1;
    if(manager->added != NULL)
        ecs_event_manager_free(manager->added);

    if(manager->removed != NULL)
        ecs_event_manager_free(manager->removed);
}

// Gets or creates the ComponentPool for a specific component type on the specified world.
static ComponentPool* ecs_component_pool_get_or_create(ComponentManager* manager, int world) {
    if(world >= manager->pool_count || manager->pools[world] == NULL) {

        ECS_ARRAY_RESIZE_DEFAULT(manager->pools, manager->pool_count, world, sizeof(*manager->pools), NULL);

        ComponentPool* result = ecs_component_pool_init(world, manager->component_size);
        manager->pools[world] = result;

        return result;
    } else {
        return manager->pools[world];
    }
}

void* ecs_component_set(Entity entity, ComponentManager* manager) {
    ComponentEnum* components;
    void* result;
    bool was_set = false;
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    ECS_ARRAY_RESIZE_DEFAULT(pool->mapping, pool->mapping_count, entity.id, sizeof(*pool->mapping), -1);

    int* index = pool->mapping + entity.id;
    if(*index != -1) {
        result = pool->components + (*index * pool->component_size);

        components = ecs_entity_get_components(entity);
        was_set = ecs_component_enum_get_flag(components, manager->flag);
        if(!was_set) {
            ecs_component_enum_set_flag(components, manager->flag, true);
            if(manager->added != NULL && ecs_component_enum_get_flag(components, is_enabled_flag)) {
                EcsComponentAddedMessage message = { entity, result };
                ecs_event_publish(entity.world, manager->added, void (*)(EcsComponentAddedMessage*), &message);
            }
        }

        if(manager->destructor != NULL)
            manager->destructor(result);

        if(manager->constructor != NULL)
            manager->constructor(result);

        return result;
    }

    *index = ++pool->last_component_index;

    ECS_ARRAY_RESIZE(pool->components, pool->component_count, pool->last_component_index, pool->component_size);

    ECS_ARRAY_RESIZE_DEFAULT(pool->links, pool->link_count, pool->last_component_index, sizeof(*pool->links), DEFAULT_COMPONENT_LINK);

    pool->links[pool->last_component_index].entity_id = entity.id;
    pool->links[pool->last_component_index].references = 1;

    result = pool->components + pool->component_size * pool->last_component_index;

    components = ecs_entity_get_components(entity);
    was_set = ecs_component_enum_get_flag(components, manager->flag);
    if(!was_set) {
        ecs_component_enum_set_flag(components, manager->flag, true);
        if(manager->added != NULL && ecs_component_enum_get_flag(components, is_enabled_flag)) {
            EcsComponentAddedMessage message = { entity, result };
            ecs_event_publish(entity.world, manager->added, void (*)(EcsComponentAddedMessage*), &message);
        }
    }

    if(manager->constructor != NULL)
        manager->constructor(result);

    return result;

}

EcsResult ecs_component_set_same_as(Entity entity, Entity reference, ComponentManager* manager) {
    if(entity.world != reference.world)
        return ECS_RESULT_DIFFERENT_WORLD;

    if(!ecs_component_exists(reference, manager))
        return ECS_RESULT_INVALID_ENTITY;

    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    ECS_ARRAY_RESIZE_DEFAULT(pool->mapping, pool->mapping_count, entity.id, sizeof(*pool->mapping), -1);

    int ref_index = pool->mapping[reference.id];
    int* index = pool->mapping + entity.id;

    if(*index != -1) {
        if(*index == ref_index)
            return ECS_RESULT_SUCCESS;

        ecs_component_remove(entity, manager);
    }

    ++pool->links[ref_index].references;
    *index = ref_index;

    ComponentEnum* components = ecs_entity_get_components(entity);
    ecs_component_enum_set_flag(components, manager->flag, true);
    if(manager->added != NULL && ecs_component_enum_get_flag(components, is_enabled_flag)) {
        void* component = pool->components + pool->component_size * ref_index;
        EcsComponentAddedMessage message = { entity, component };
        ecs_event_publish(entity.world, manager->added, void (*)(EcsComponentAddedMessage*), &message);
    }

    return ECS_RESULT_SUCCESS;
}

EcsResult ecs_component_remove(Entity entity, ComponentManager* manager) {
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return ECS_RESULT_INVALID_ENTITY;

    int* index = pool->mapping + entity.id;
    if(*index == -1)
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = ecs_entity_get_components(entity);
    bool was_set = ecs_component_enum_get_flag(components, manager->flag);
    if(was_set) {
        ecs_component_enum_set_flag(components, manager->flag, false);
        if(manager->removed != NULL && ecs_component_enum_get_flag(components, is_enabled_flag)) {
            void* component = pool->components + pool->component_size * *index;
            EcsComponentRemovedMessage message = { entity, component };
            ecs_event_publish(entity.world, manager->removed, void (*)(EcsComponentRemovedMessage*), &message);
        }
    }

    ComponentLink* link = pool->links + *index;
    if(--link->references == 0) {
        if(manager->destructor != NULL)
            manager->destructor(pool->components + (pool->component_size * *index));

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

EcsResult ecs_component_enable(Entity entity, ComponentManager* manager) {
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return ECS_RESULT_INVALID_ENTITY;

    int index;
    if((index = pool->mapping[entity.id]) == -1)
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = ecs_entity_get_components(entity);

    if(!ecs_component_enum_get_flag(components, manager->flag)) {
        void* component = pool->components + pool->component_size * index;
        EcsComponentAddedMessage message = { entity, component };
        ecs_component_enum_set_flag(components, manager->flag, true);
        if(manager->added != NULL)
            ecs_event_publish(entity.world, manager->added, void (*)(EcsComponentAddedMessage*), &message);

        return ECS_RESULT_SUCCESS;
    }

    return ECS_RESULT_INVALID_STATE;
}

EcsResult ecs_component_disable(Entity entity, ComponentManager* manager) {
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return ECS_RESULT_INVALID_ENTITY;

    int index;
    if((index = pool->mapping[entity.id]) == -1)
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = ecs_entity_get_components(entity);

    if(ecs_component_enum_get_flag(components, manager->flag)) {
        void* component = pool->components + pool->component_size * index;
        EcsComponentRemovedMessage message = { entity, component };
        ecs_component_enum_set_flag(components, manager->flag, false);
        if(manager->removed != NULL)
            ecs_event_publish(entity.world, manager->removed, void (*)(EcsComponentRemovedMessage*), &message);

        return ECS_RESULT_SUCCESS;
    }

    return ECS_RESULT_INVALID_STATE;
}

bool ecs_component_is_enabled(Entity entity, ComponentManager* manager) {
    if(!ecs_component_exists(entity, manager))
        return ECS_RESULT_INVALID_ENTITY;

    ComponentEnum* components = ecs_entity_get_components(entity);
    return ecs_component_enum_get_flag(components, is_enabled_flag) && ecs_component_enum_get_flag(components, manager->flag);
}

EcsResult ecs_component_get(Entity entity, ComponentManager* manager, void** data) {
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return ECS_RESULT_INVALID_ENTITY;

    int index = pool->mapping[entity.id];
    if(index == -1)
        return ECS_RESULT_INVALID_ENTITY;

    *data = pool->components + (index * pool->component_size);
    return ECS_RESULT_SUCCESS;
}

bool ecs_component_exists(Entity entity, ComponentManager* manager) {
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, entity.world);

    if(entity.id >= pool->mapping_count)
        return false;

    return pool->mapping[entity.id] != -1;
}

void* ecs_component_get_all(EcsWorld world, ComponentManager* manager, int* count) {
    ComponentPool* pool = ecs_component_pool_get_or_create(manager, world);
    *count = pool->last_component_index + 1;
    return pool->components;
}

void ecs_component_iter_enabled_start(EcsWorld world, ComponentManager* manager, ComponentIterator* iterator) {
    int count;
    iterator->pool = ecs_component_pool_get_or_create(manager, world);
    iterator->components = ecs_world_get_components(world, &count);
    iterator->index = -1;
    iterator->flag = COMPONENT_ENUM_DEFAULT;
    ecs_component_enum_set_flag(&iterator->flag, is_enabled_flag, true);
    ecs_component_enum_set_flag(&iterator->flag, manager->flag, true);
}

bool ecs_component_iter_enabled_next(ComponentIterator* iterator, void** result) {
    while(++iterator->index <= iterator->pool->last_component_index) {
        if(!ecs_component_enum_contains_enum(iterator->components + iterator->index, &iterator->flag))
            continue;

        *result = iterator->pool->components + (iterator->index * iterator->pool->component_size);
        return true;
    }

    return false;
}

void ecs_component_iter_enabled_end(ComponentIterator* iterator) {
    ecs_component_enum_free_resources(&iterator->flag);
}