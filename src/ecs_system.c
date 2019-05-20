#include "ecs_system.h"
#include "ecs_world.h"

void ecs_system_init(EcsSystem* system, EcsSystemType type, EcsSystemPreupdate preupdate, EcsSystemPostupdate postupdate) {
    system->type = type;
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
    ecs_component_enum_free_resources(&entity_system->with);
    ecs_component_enum_free_resources(&entity_system->without);
}

void ecs_system_free_resources(EcsSystem* system) {
    ecs_event_trigger(system->dispose, void (*)(void*, EcsSystem*), system);
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
                               ComponentManager* component_type, 
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
                            ComponentEnum* with, 
                            ComponentEnum* without, 
                            EcsSystemUpdateEntity update, 
                            EcsSystemPreupdate preupdate, 
                            EcsSystemPostupdate postupdate)
{
    ecs_system_init(&system->base, ECS_SYSTEM_TYPE_ENTITY, preupdate, postupdate);
    system->world = world;
    system->with = with == NULL ? COMPONENT_ENUM_DEFAULT : ecs_component_enum_copy(with);
    ecs_component_enum_set_flag(&system->with, is_enabled_flag, true);
    system->without = without == NULL ? COMPONENT_ENUM_DEFAULT : ecs_component_enum_copy(without);
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

    int component_count;

    switch(system->type) 
    {
        case ECS_SYSTEM_TYPE_ACTION:
            EcsSystemUpdateAction action = ((EcsActionSystem*)system)->update;
            if(action != NULL)
                action((EcsActionSystem*)system, delta_time);
            break;

        case ECS_SYSTEM_TYPE_SEQUENTIAL:
            EcsSequentialSystem* seq_system = (EcsSequentialSystem*)system;
            for(int i = 0; i < seq_system->count; ++i) {
                ecs_system_update(seq_system->systems[i], delta_time);
            }
            break;

        case ECS_SYSTEM_TYPE_COMPONENT:
            EcsComponentSystem* component_system = (EcsComponentSystem*)system;
            if(component_system->update == NULL)
                break;

            void* item;
            ECS_COMPONENT_ITERATE_ENABLED_START(component_system->world, component_system->manager, &item)

                component_system->update(component_system, delta_time, item);

            ECS_COMPONENT_ITERATE_ENABLED_END()
            break;

        case ECS_SYSTEM_TYPE_ENTITY:
            // Todo: Create EntitySet struct that updates when entities are created/destroyed or components are added/removed
            //       to increase the speed of this type of system.

            EcsEntitySystem* entity_system = (EcsEntitySystem*)system;
            if(entity_system->update == NULL)
                break;

            Entity entity;
            entity.world = entity_system->world;
            ComponentEnum* entity_components = ecs_world_get_components(entity_system->world, &component_count);

            for(int i = 0; i < component_count; i++) {
                if(ecs_component_enum_contains_enum(entity_components, &entity_system->with)
                    && !ecs_component_enum_contains_enum(entity_components, &entity_system->without)) 
                {
                    entity.id = i;
                    entity_system->update(entity_system, delta_time, entity);
                }
                entity_components++;
            }
            break;
    }

    if(system->postupdate != NULL)
        system->postupdate(system, delta_time);
}