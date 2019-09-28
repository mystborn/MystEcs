#include <ecs_system.h>
#include <ecs_world.h>

void ecs_system_init(EcsSystem* system, EcsSystemType type, EcsSystemPreupdate preupdate, EcsSystemPostupdate postupdate) {
    system->type = type;
    system->enabled = true;
    system->preupdate = preupdate;
    system->postupdate = postupdate;
    system->dispose = ecs_event_init();
}

static void ecs_sequential_system_free(void* data, EcsSystem* system) {
    EcsSequentialSystem* seq_system = (EcsSequentialSystem*)system;
    if(seq_system->free_children) {
        for(int i = 0; i < seq_system->count; ++i) {
            if(seq_system->systems[i] != NULL) {
                ecs_system_free_resources(seq_system->systems[i]);
                ecs_free(seq_system->systems[i]);
            }
        }
    }
    ecs_free(seq_system->systems);
}

static void ecs_entity_system_free(void* data, EcsSystem* system) {
    EcsEntitySystem* entity_system = (EcsEntitySystem*)system;
    ecs_entity_set_free(entity_system->entities);
}

void ecs_system_free_resources(EcsSystem* system) {
    ecs_event_trigger(system->dispose, void (*)(void*, EcsSystem*), system);
    ecs_event_free(system->dispose);
}

bool ecs_system_enable(EcsSystem* system) {
    if(system->enabled)
        return false;
    system->enabled = true;
    return true;
}

bool ecs_system_disable(EcsSystem* system) {
    if(!system->enabled)
        return false;

    system->enabled = false;
    return true;
}

void ecs_component_system_init(EcsComponentSystem* system, 
                               EcsWorld world, 
                               EcsComponentManager* component_type, 
                               EcsSystemUpdateComponent update, 
                               EcsSystemPreupdate preupdate, 
                               EcsSystemPostupdate postupdate)
{
    ecs_system_init(&system->base, ECS_SYSTEM_TYPE_COMPONENT, preupdate, postupdate);
    system->world = world;
    system->manager = component_type;
    system->update = update;
}

void ecs_entity_system_init(EcsEntitySystem* system, 
                            EcsWorld world,
                            EcsEntitySetBuilder* builder,
                            bool free_builder,
                            EcsSystemUpdateEcsEntity update, 
                            EcsSystemPreupdate preupdate, 
                            EcsSystemPostupdate postupdate)
{
    ecs_system_init(&system->base, ECS_SYSTEM_TYPE_ENTITY, preupdate, postupdate);
    system->world = world;
    system->entities = ecs_entity_set_build(builder, world, free_builder);
    system->update = update;
    ecs_event_add(system->base.dispose, ecs_closure(NULL, ecs_entity_system_free));
}

void ecs_action_system_init(EcsActionSystem* system, 
                            EcsSystemUpdateAction update, 
                            EcsSystemPreupdate preupdate, 
                            EcsSystemPostupdate postupdate)
{
    ecs_system_init((EcsSystem*)system, ECS_SYSTEM_TYPE_ACTION, preupdate, postupdate);
    system->update = update;
}

void ecs_sequential_system_init(EcsSequentialSystem* system, 
                                EcsSystemPreupdate preupdate, 
                                EcsSystemPostupdate postupdate, 
                                bool free_children,
                                int count, 
                                ...)
{
    ecs_system_init(&system->base, ECS_SYSTEM_TYPE_SEQUENTIAL, preupdate, postupdate);
    EcsSystem** systems = ecs_malloc(sizeof(EcsSystem*) * count);
    va_list args;
    va_start(args, count);
    for(int i = 0; i < count; i++) {
        systems[i] = va_arg(args, EcsSystem*);
    }
    va_end(args);
    system->systems = systems;
    system->count = count;
    system->free_children = free_children;
    ecs_event_add(system->base.dispose, ecs_closure(NULL, ecs_sequential_system_free));
}

void ecs_sequential_system_init_array(EcsSequentialSystem* system, 
                                      EcsSystemPreupdate preupdate, 
                                      EcsSystemPostupdate postupdate,
                                      bool free_children,
                                      int count,
                                      EcsSystem** systems) 
{
    ecs_system_init(&system->base, ECS_SYSTEM_TYPE_SEQUENTIAL, preupdate, postupdate);
    system->systems = malloc(count * sizeof(EcsSystem*));
    ecs_memcpy(system->systems, systems, count * sizeof(EcsSystem*));
    system->count = count;
    system->free_children = free_children;
    ecs_event_add(system->base.dispose, ecs_closure(NULL, ecs_sequential_system_free));
}

void ecs_sequential_system_init_list(EcsSequentialSystem* system, 
                                     EcsSystemPreupdate preupdate, 
                                     EcsSystemPostupdate postupdate,
                                     bool free_children,
                                     int count,
                                     va_list list)
{
    ecs_system_init(&system->base, ECS_SYSTEM_TYPE_SEQUENTIAL, preupdate, postupdate);
    system->systems = malloc(count * sizeof(EcsSystem*));
    for(int i = 0; i < count; i++) {
        system->systems[i] = va_arg(list, EcsSystem*);
    }
    system->count = count;
    system->free_children = free_children;
    ecs_event_add(system->base.dispose, ecs_closure(NULL, ecs_sequential_system_free));
}

void ecs_system_update(EcsSystem* system, float delta_time) {
    if(!system->enabled)
        return;

    if(system->preupdate != NULL)
        system->preupdate(system, delta_time);

    switch(system->type) 
    {
        case ECS_SYSTEM_TYPE_ACTION:
        {
            EcsSystemUpdateAction action = ((EcsActionSystem*)system)->update;
            if(action != NULL)
                action((EcsActionSystem*)system, delta_time);
            break;
        }

        case ECS_SYSTEM_TYPE_SEQUENTIAL:
        {
            EcsSequentialSystem* seq_system = (EcsSequentialSystem*)system;
            for(int i = 0; i < seq_system->count; ++i) {
                ecs_system_update(seq_system->systems[i], delta_time);
            }
            break;
        }
        case ECS_SYSTEM_TYPE_COMPONENT:
        {
            EcsComponentSystem* component_system = (EcsComponentSystem*)system;
            if(component_system->update == NULL)
                break;

            // The items array has to be of type char* because you can't increment a void ptr.
            int component_count;
            char* items = ecs_component_get_all(component_system->world, component_system->manager, &component_count);

            for(int i = 0; i < component_count; i++)
                component_system->update(component_system, delta_time, &items[i * component_system->manager->component_size]);

            break;
        }
        case ECS_SYSTEM_TYPE_ENTITY:
        {
            // Originally this iterated over the components of all entities
            // using ecs_world_get_components. The system had a with and without
            // ComponentEnum field and for each entity it would compare against the fields.
            // That version is actually FASTER if the components are being changed frequently
            // (i.e. like every frame). Using an EcsEntitySet is faster when the components aren't changing,
            // which is the more general case, so it's used instead. If this is too large of a bottleneck,
            // a hybrid solution may be applicable.

            EcsEntitySystem* entity_system = (EcsEntitySystem*)system;
            if(entity_system->update == NULL)
                break;

            int entity_count;
            EcsEntity* entities = ecs_entity_set_get_entities(entity_system->entities, &entity_count);

            for(int i = 0; i < entity_count; i++)
                entity_system->update(entity_system, delta_time, entities[i]);

            break;
        }
    }

    if(system->postupdate != NULL)
        system->postupdate(system, delta_time);
}