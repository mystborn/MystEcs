#include <ecs_recorder.h>
#include <ecs_array.h>

typedef enum EcsCommandType {
    COMMAND_ENTITY,
    COMMAND_CREATE_ENTITY,
    COMMAND_ENABLE,
    COMMAND_DISABLE,
    COMMAND_SET,
    COMMAND_SET_SAME_AS,
    COMMAND_REMOVE,
    COMMAND_FREE
} EcsCommandType;

struct EcsCommandEntity {
    EcsEntity entity;
};

struct EcsCommandCreateEntity {
    // This starts as an entity with an id of 0,
    // and when the command is run the id is replaced with the
    // command result.
    EcsEntity entity;
};

struct EcsCommandEntityEnable {
    int entity_offset;
};

struct EcsCommandEntityDisable {
    int entity_offset;
};

struct EcsCommandEntitySet {
    int entity_offset;
    EcsComponentManager* component;
    void* value;
};

struct EcsCommandEntitySetSameAs {
    int entity_offset;
    int reference_offset;
    EcsComponentManager* component;
};

struct EcsCommandEntityRemove {
    int entity_offset;
    EcsComponentManager* component;
};

struct EcsCommandEntityFree {
    int entity_offset;
};

ECS_EXPORT void ecs_recorder_init(EcsRecorder* recorder) {
    recorder->buffer = NULL;
    recorder->count = 0;
    recorder->capacity = 0;
}

ECS_EXPORT EcsRecorder* ecs_recorder_create(void) {
    EcsRecorder* recorder = ecs_malloc(sizeof(*recorder));
    ecs_recorder_init(recorder);
    return recorder;
}

ECS_EXPORT void ecs_recorder_free_resources(EcsRecorder* recorder) {
    ecs_free(recorder->buffer);
}

ECS_EXPORT void ecs_recorder_free(EcsRecorder* recorder) {
    ecs_recorder_free_resources(recorder);
    ecs_free(recorder);
}

static inline EcsEntity ecs_recorder_get_entity(EcsRecorder* recorder, int offset) {
    return *(EcsEntity*)(recorder->buffer + offset);
}

void ecs_recorder_execute(EcsRecorder* recorder) {
    int index = 0;
    while (index < recorder->count) {
        switch (recorder->buffer[index++]) {
            case COMMAND_ENTITY:
                break;
            case COMMAND_CREATE_ENTITY:
                struct EcsCommandCreateEntity* create_command = (struct EcsCommandCreateEntity*)recorder->buffer;
                create_command->entity = ecs_create_entity(create_command->entity.world);
                index += sizeof(*create_command);
                break;
            case COMMAND_ENABLE:
                struct EcsCommandEntityEnable* enable_command = (struct EcsCommandEntityEnable*)recorder->buffer;
                ecs_entity_enable(ecs_recorder_get_entity(recorder, enable_command->entity_offset));
                index += sizeof(*enable_command);
                break;
            case COMMAND_DISABLE:
                struct EcsCommandEntityDisable* disable_command = (struct EcsCommandEntityDisable*)recorder->buffer;
                ecs_entity_disable(ecs_recorder_get_entity(recorder, disable_command->entity_offset));
                index += sizeof(*disable_command);
                break;
            case COMMAND_SET:
                struct EcsCommandEntitySet* set_command = (struct EcsCommandEntitySet*)recorder->buffer;
                ecs_entity_set(ecs_recorder_get_entity(recorder, set_command->entity_offset), set_command->component, set_command->value);
                index += sizeof(*set_command);
                break;
            case COMMAND_SET_SAME_AS:
                struct EcsCommandEntitySetSameAs* set_same_as_command = (struct EcsCommandEntitySetSameAs*)recorder->buffer;
                ecs_entity_set_same_as(
                    ecs_recorder_get_entity(recorder, set_same_as_command->entity_offset),
                    ecs_recorder_get_entity(recorder, set_same_as_command->reference_offset),
                    set_same_as_command->component);
                index += sizeof(*set_same_as_command);
                break;
            case COMMAND_REMOVE:
                struct EcsCommandEntityRemove* remove_command = (struct EcsCommandEntityRemove*)recorder->buffer;
                ecs_entity_remove(ecs_recorder_get_entity(recorder, remove_command->entity_offset), remove_command->component);
                index += sizeof(*remove_command);
                break;
            case COMMAND_FREE:
                struct EcsCommandEntityFree* free_command = (struct EcsCommandEntityFree*)recorder->buffer;
                ecs_entity_free(ecs_recorder_get_entity(recorder, free_command->entity_offset));
                index += sizeof(*free_command);
                break;
            default:
                puts("Panic! Entity recorder has ended up in an unknown state!");
                puts("Trying to recover...");
                index += 1;
                break;
        }
    }
}

#define RECORD_COMMAND(recorder, command_type, command) \
    ECS_ARRAY_RESIZE(recorder->buffer, recorder->capacity, recorder->count + sizeof(command) + 1, sizeof(char)); \
    recorder->buffer[recorder->count++] = command_type; \
    ecs_memcpy(recorder->buffer + recorder->count, &command, sizeof(command)); \
    recorder->count += sizeof(command); \


ECS_EXPORT EcsRecordEntity ecs_record_create_entity(EcsRecorder* recorder, EcsWorld world) {
    struct EcsCommandCreateEntity command = (struct EcsCommandCreateEntity){ (EcsEntity){ .world = world } };
    int offset = recorder->count + 1;
    RECORD_COMMAND(recorder, COMMAND_CREATE_ENTITY, command);
    return (EcsRecordEntity){ recorder, offset };
}

ECS_EXPORT EcsRecordEntity ecs_record_entity(EcsRecorder* recorder, EcsEntity entity) {
    struct EcsCommandEntity command = (struct EcsCommandEntity){ entity };
    int offset = recorder->count + 1;
    RECORD_COMMAND(recorder, COMMAND_ENTITY, command);
    return (EcsRecordEntity){ recorder, offset };
}

ECS_EXPORT void ecs_record_entity_enable(EcsRecordEntity entity) {
    struct EcsCommandEntityEnable command = (struct EcsCommandEntityEnable){ entity.offset };
    RECORD_COMMAND(entity.recorder, COMMAND_ENABLE, command);
}

ECS_EXPORT void ecs_record_entity_disable(EcsRecordEntity entity) {
    struct EcsCommandEntityDisable command = (struct EcsCommandEntityDisable){ entity.offset };
    RECORD_COMMAND(entity.recorder, COMMAND_DISABLE, command);
}
ECS_EXPORT void ecs_record_entity_set(EcsRecordEntity entity, EcsComponentManager* component, void* value) {
    struct EcsCommandEntitySet command = (struct EcsCommandEntitySet){ entity.offset, component, value };
    RECORD_COMMAND(entity.recorder, COMMAND_SET, command);
}

ECS_EXPORT void ecs_record_entity_set_same_as(EcsRecordEntity entity, EcsRecordEntity reference, EcsComponentManager* component) {
    assert(entity.recorder == reference.recorder);

    struct EcsCommandEntitySet command = (struct EcsCommandEntitySet){ entity.offset, reference.offset, component };
    RECORD_COMMAND(entity.recorder, COMMAND_SET_SAME_AS, command);
}

ECS_EXPORT void ecs_record_entity_remove(EcsRecordEntity entity, EcsComponentManager* component) {
    struct EcsCommandEntityRemove command = (struct EcsCommandEntityRemove){ entity.offset, component };
    RECORD_COMMAND(entity.recorder, COMMAND_REMOVE, command);
}

ECS_EXPORT void ecs_record_entity_free(EcsRecordEntity entity) {
    struct EcsCommandEntityFree command = (struct EcsCommandEntityFree){ entity.offset };
    RECORD_COMMAND(entity.recorder, COMMAND_FREE, command);
}