#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

typedef int EcsWorld;

typedef struct Entity {
    int world;
    int id;
} Entity;

#endif