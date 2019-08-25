#ifndef ECS_ECS_MESSAGES_H
#define ECS_ECS_MESSAGES_H

// This file defines various messages used by events.

#include "component.h"
#include "ecs_common.h"
#include "entity.h"
#include "ecs_event.h"

/// Message sent when an EcsEntity is created.
typedef struct EcsEcsEntityCreatedMessage {
    /// The entity that was created.
    EcsEntity entity;
} EcsEcsEntityCreatedMessage;

/// Message sent when an EcsEntity is freed.
typedef struct EcsEcsEntityDisposedMessage {
    /// The entity that was freed.
    EcsEntity entity;
} EcsEcsEntityDisposedMessage;

/// Message sent when an EcsEntity is enabled.
typedef struct EcsEcsEntityEnabledMessage {
    /// The entity that was enabled.
    EcsEntity entity;
} EcsEcsEntityEnabledMessage;

/// Message sent when an EcsEntity is disabled.
typedef struct EcsEcsEntityDisabledMessage {
    /// The entity that was disabled.
    EcsEntity entity;
} EcsEcsEntityDisabledMessage;

// Todo: Remove the component field from 
//       EcsComponentAddedMessage and EcsComponentRemovedMessage.

/// Message sent when a component is added to an entity. 
/// Will be reworked in the near future, so use with caution.
typedef struct EcsComponentAddedMessage {
    /// The entity that had a component added to.
    EcsEntity entity;

    /// The type of the component added to the entity.
    EcsComponentManager* component_type;

    /// The actual component added to the entity.
    void* component;
} EcsComponentAddedMessage;

/// Message set when a component is removed from an entity. 
/// Will be reworked in the near future, so use with caution.
typedef struct EcsComponentRemovedMessage {
    /// The entity that the component was removed from.
    EcsEntity entity;

    /// The type of the component that was removed.
    EcsComponentManager* component_type;

    /// The actual value of the component that was removed.
    void* component;
} EcsComponentRemovedMessage;

/// Message sent when a world is freed.
typedef struct EcsWorldDisposedMessage {
    /// The world that was freed.
    EcsWorld world;
} EcsWorldDisposedMessage;

/// Event manager that is triggered when an entity is created.
extern EcsEventManager* ecs_entity_created;

/// Event manager that is triggered when an entity is freed.
extern EcsEventManager* ecs_entity_disposed;

/// Event manager that is triggered when an entity is enabled.
extern EcsEventManager* ecs_entity_enabled;

/// Event manager that is triggered when an entity is disabled.
extern EcsEventManager* ecs_entity_disabled;

/// An event that is triggered when a world is freed.
extern EcsEvent* ecs_world_disposed;

/// Initializes the various events that utilize the messages. Should not be called directly.
void ecs_messages_init(void);


#endif