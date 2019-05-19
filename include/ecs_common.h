#ifndef ECS_ECS_COMMON_H
#define ECS_ECS_COMMON_H

// Bad practice, but includes and defines files and functions needed by essentially every file.

#include <stdbool.h>
#include <stddef.h>
#include "ecs_array.h"

// Originally these were defined to use the SDL version of each function if the library was
// availabe, however I ran into a bug with SDL_malloc or SDL_realloc that was overwriting valid
// information, so now we stick to the tried and true std versions.

// For anyone wondering, the place where I encountered the bug was in 'ecs_event.c' when
// a EcsEventManager would initialize a new EcsEvent, for some reason
// the 'subscriptions' field of the event had the same address as the event manager.

// It only appeared when I changed the initial starting array size from 8 to 4, and I
// couldn't solve the issue, so I scrapped the SDL integration completely. Using the 
// stdlib fixed the issue without changing any code, so that's what I'm sticking with.

#include <stdlib.h>
#include <string.h>
#define ecs_malloc malloc
#define ecs_realloc realloc
#define ecs_free free
#define ecs_memmove memmove
#define ecs_memcpy memcpy
#define ecs_memset memset

typedef enum EcsResult {
    ECS_RESULT_SUCCESS,
    ECS_RESULT_DIFFERENT_WORLD,
    ECS_RESULT_INVALID_ENTITY,
    ECS_RESULT_INVALID_STATE,
    ECS_RESULT_INVALID_WORLD
} EcsResult;

#endif