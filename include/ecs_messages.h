#ifndef ECS_ECS_MESSAGES_H
#define ECS_ECS_MESSAGES_H

// This file defines various messages used by events.

#include "component.h"
#include "ecs_common.h"
#include "entity.h"
#include "ecs_event.h"

// Message sent when an Entity is created.
typedef struct EcsEntityCreatedMessage {
    Entity entity;
} EcsEntityCreatedMessage;

// Message sent when an Entity is freed.
typedef struct EcsEntityDisposedMessage {
    Entity entity;
} EcsEntityDisposedMessage;

// Message sent when an Entity is enabled.
typedef struct EcsEntityEnabledMessage {
    Entity entity;
} EcsEntityEnabledMessage;

// Message sent when an Entity is disabled.
typedef struct EcsEntityDisabledMessage {
    Entity entity;
} EcsEntityDisabledMessage;

// Message sent when a component is added to an entity. 
// Will be reworked in the near future, so use with caution.
typedef struct EcsComponentAddedMessage {
    Entity entity;
    void* component;
} EcsComponentAddedMessage;

// Message set when a component is removed from an entity. 
// Will be reworked in the near future, so use with caution.
typedef struct EcsComponentRemovedMessage {
    Entity entity;
    void* component;
} EcsComponentRemovedMessage;

// Message sent when a world is freed.
typedef struct EcsWorldDisposedMessage {
    EcsWorld world;
} EcsWorldDisposedMessage;

// Ecs Event that is triggered when an entity is created.
extern EcsEventManager* ecs_entity_created;

// Ecs Event that is triggered when an entity is freed.
extern EcsEventManager* ecs_entity_disposed;

// Ecs Event that is triggered when an entity is enabled.
extern EcsEventManager* ecs_entity_enabled;

// Ecs Event that is triggered when an entity is disabled.
extern EcsEventManager* ecs_entity_disabled;

// Initializes the various events that utilize the messages.
void ecs_messages_init(void);


#endif