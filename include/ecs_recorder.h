#ifndef ECS_RECORDER_H
#define ECS_RECORDER_H

#include "ecs_common.h"
#include "ecs_world.h"
#include "ecs_entity.h"
#include "ecs_component.h"

typedef struct EcsRecorder {
    char* buffer;
    int count;
    int capacity;
} EcsRecorder;

typedef struct EcsRecordWorld {
    EcsRecorder* recorder;
    EcsWorld world;
} EcsRecordWorld;

typedef struct EcsRecordEntity {
    EcsRecorder* recorder;
    int offset;
} EcsRecordEntity;

ECS_EXPORT void ecs_recorder_init(EcsRecorder* recorder);
ECS_EXPORT EcsRecorder* ecs_recorder_create(void);
ECS_EXPORT void ecs_recorder_free_resources(EcsRecorder* recorder);
ECS_EXPORT void ecs_recorder_free(EcsRecorder* recorder);

void ecs_recorder_execute(EcsRecorder* recorder);

ECS_EXPORT EcsRecordEntity ecs_record_create_entity(EcsRecorder* recorder, EcsWorld world);
ECS_EXPORT EcsRecordEntity ecs_record_entity(EcsRecorder* recorder, EcsEntity entity);
ECS_EXPORT void ecs_record_entity_enable(EcsRecordEntity entity);
ECS_EXPORT void ecs_record_entity_disable(EcsRecordEntity entity);
ECS_EXPORT void ecs_record_entity_set(EcsRecordEntity entity, EcsComponentManager* component, void* value);
ECS_EXPORT void ecs_record_entity_set_same_as(EcsRecordEntity entity, EcsRecordEntity reference, EcsComponentManager* component);
ECS_EXPORT void ecs_record_entity_remove(EcsRecordEntity entity);
ECS_EXPORT void ecs_record_entity_free(EcsRecordEntity entity);


#endif