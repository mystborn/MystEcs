/*!
 * @file
 *
 * \brief Includes all necessary files to use ecs.
 *
 * A file that imports all of ecs in one header. 
 * These types of files are generally frowned upon, 
 * but I've found that it barely adds any compilation time.
 */
#ifndef ECS_ECS_H
#define ECS_ECS_H

#include "ecs_common.h"
#include "ecs_entity.h"
#include "ecs_int_dispenser.h"
#include "ecs_component_flag.h"
#include "ecs_component.h"
#include "ecs_messages.h"
#include "ecs_system.h"
#include "ecs_event.h"
#include "ecs_world.h"
#include "ecs_entity_set.h"

/// Initializes the various systems needed to use ecs.
ECS_EXPORT void ecs_init(void);

#endif //ECS_ECS_H