#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

/// A world that can create entities, components, and systems.
typedef int EcsWorld;

/// The entity part of entity-component-system.
typedef struct Entity {
    /// The world that the entity belongs to.
    int world;

    /// The id of the entity inside of the world.
    int id;
} Entity;

#endif