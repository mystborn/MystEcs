#ifndef ECS_ENTITY_SET_H
#define ECS_ENTITY_SET_H

#include "ecs_world.h"
#include "component.h"

typedef struct EntitySetBuilder EntitySetBuilder;
typedef struct EntitySet EntitySet;

EntitySetBuilder* ecs_entity_set_builder_init(void);
void ecs_entity_set_builder_free(EntitySetBuilder* builder);
void ecs_entity_set_with(EntitySetBuilder* builder, ComponentManager* manager);
void ecs_entity_set_without(EntitySetBuilder* builder, ComponentManager* manager);

EntitySet* ecs_entity_set_build(EntitySetBuilder* builder, bool free_builder);
void ecs_entity_set_free(EntitySet* set);
Entity* ecs_entity_set_get_entities(EntitySet* set, int* count);

#endif