#include "check.h"
#include "ecs.h"

#include <stdio.h>

EcsComponentManager* bool_component;
EcsComponentManager* int_component;

int action_count;

void action_update(EcsActionSystem* system, float state) {
    action_count++;
}

void component_update(EcsComponentSystem* system, float state, void* item) {
    *(bool*)item = true;
}

void entity_update(EcsEntitySystem* system, float state, EcsEntity entity) {
    bool* item;
    ecs_entity_get(entity, bool_component, &item);
    *item = true;
}

void bool_component_constructor(void* item) {
    *(bool*)item = false;
}

void system_setup(void) {
    ecs_init();
    bool_component = ecs_component_define(sizeof(bool), bool_component_constructor, NULL);
    int_component = ecs_component_define(sizeof(int), NULL, NULL);
}

void system_teardown(void) {
    ecs_component_free(bool_component);
    ecs_component_free(int_component);
}

void system_start(void) {
    action_count = 0;
}

START_TEST(action_enabled_update_calls) {
    EcsActionSystem system;
    ecs_action_system_init(&system, action_update, NULL, NULL);
    ecs_system_update((EcsSystem*)&system, 0);
    ck_assert_msg(action_count == 1, "Action system did not update when enabled");
    ecs_system_free_resources((EcsSystem*)&system);
}
END_TEST

START_TEST(action_disabled_update_not_calls) {
    EcsActionSystem system;
    ecs_action_system_init(&system, action_update, NULL, NULL);
    ecs_system_disable((EcsSystem*)&system);
    ecs_system_update((EcsSystem*)&system, 0);
    ck_assert_msg(action_count == 0, "Action system updated when disabled");
    ecs_system_free_resources((EcsSystem*)&system);
}
END_TEST

START_TEST(sequential_enabled_update_calls_enabled_all) {
    EcsActionSystem a1, a2;
    EcsSequentialSystem system;

    ecs_action_system_init(&a1, action_update, NULL, NULL);
    ecs_action_system_init(&a2, action_update, NULL, NULL);

    ecs_sequential_system_init(&system, NULL, NULL, false, 2, &a1, &a2);
    ecs_system_update((EcsSystem*)&system, 0);

    ck_assert_msg(action_count == 2, "Sequential system failed to call all children");
    ecs_system_free_resources((EcsSystem*)&a1);
    ecs_system_free_resources((EcsSystem*)&a2);
    ecs_system_free_resources((EcsSystem*)&system);
}
END_TEST

START_TEST(sequential_enabled_update_calls_enabled_some) {
    EcsActionSystem a1, a2;
    EcsSequentialSystem system;

    ecs_action_system_init(&a1, action_update, NULL, NULL);
    ecs_action_system_init(&a2, action_update, NULL, NULL);
    ecs_system_disable((EcsSystem*)&a1);

    ecs_sequential_system_init(&system, NULL, NULL, false, 2, &a1, &a2);
    ecs_system_update((EcsSystem*)&system, 0);

    ck_assert_msg(action_count == 1, "Sequential system failed to call all children");
    ecs_system_free_resources((EcsSystem*)&a1);
    ecs_system_free_resources((EcsSystem*)&a2);
    ecs_system_free_resources((EcsSystem*)&system);
}
END_TEST

START_TEST(sequential_disabled_update_calls_none) {
    EcsActionSystem a1, a2;
    EcsSequentialSystem system;

    ecs_action_system_init(&a1, action_update, NULL, NULL);
    ecs_action_system_init(&a2, action_update, NULL, NULL);

    ecs_sequential_system_init(&system, NULL, NULL, false, 2, &a1, &a2);
    ecs_system_disable((EcsSystem*)&system);

    ecs_system_update((EcsSystem*)&system, 0);

    ck_assert_msg(action_count == 0, "Sequential system updated when disabled");
    ecs_system_free_resources((EcsSystem*)&a1);
    ecs_system_free_resources((EcsSystem*)&a2);
    ecs_system_free_resources((EcsSystem*)&system);
}
END_TEST

START_TEST(component_enabled_update_calls) {
    puts("moo");
    EcsWorld world = ecs_world_init();
    EcsEntity entity1 = ecs_create_entity(world);
    EcsEntity entity2 = ecs_create_entity(world);
    EcsEntity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_entity_set(entity1, bool_component);
    bool* c2 = ecs_entity_set(entity2, bool_component);
    bool* c3 = ecs_entity_set(entity3, bool_component);
    EcsComponentSystem system;
    ecs_component_system_init(&system, world, bool_component, component_update, NULL, NULL);
    ecs_system_update((EcsSystem*)&system, 0);
    ck_assert_msg(*c1 && *c2 && *c3, "Component system failed to update for all components");
    ecs_system_free_resources((EcsSystem*)&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(component_disabled_update_not_calls) {
    EcsWorld world = ecs_world_init();
    EcsEntity entity1 = ecs_create_entity(world);
    EcsEntity entity2 = ecs_create_entity(world);
    EcsEntity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_entity_set(entity1, bool_component);
    bool* c2 = ecs_entity_set(entity2, bool_component);
    bool* c3 = ecs_entity_set(entity3, bool_component);
    EcsComponentSystem system;
    ecs_component_system_init(&system, world, bool_component, component_update, NULL, NULL);
    ecs_system_disable(&system);
    ecs_system_update(&system, 0);
    ck_assert_msg(!(*c1 || *c2 || *c3), "Component system updated components when disabled");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

// START_TEST(component_ignores_disabled_entities) {
//     puts("moo 1.75");
//     EcsWorld world = ecs_world_init();
//     EcsEntity entity = ecs_create_entity(world);
//     bool* component = ecs_entity_set(entity, bool_component);
//     EcsComponentSystem system;
//     ecs_component_system_init(&system, world, bool_component, component_update, NULL, NULL);
//     ecs_system_update((EcsSystem*)&system, 0);
//     ck_assert_msg(*component, "Component system failed to update a component");
//     *component = false;
//     ecs_entity_disable(entity);
//     ecs_system_update((EcsSystem*)&system, 0);
//     ck_assert_msg(!(*component), "Component system updated entity that was disabled");
//     ecs_system_free_resources((EcsSystem*)&system);
//     ecs_world_free(world);
// }
// END_TEST

START_TEST(entity_enabled_update_all) {
    puts("moo 2");
    EcsEntitySetBuilder* builder = ecs_entity_set_builder_init();
    ecs_entity_set_with(builder, bool_component);
    ecs_entity_set_without(builder, int_component);

    EcsWorld world = ecs_world_init();
    EcsEntity entity1 = ecs_create_entity(world);
    EcsEntity entity2 = ecs_create_entity(world);
    EcsEntity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_entity_set(entity1, bool_component);
    bool* c2 = ecs_entity_set(entity2, bool_component);
    bool* c3 = ecs_entity_set(entity3, bool_component);

    EcsEntitySystem system = (EcsEntitySystem){0};
    ecs_entity_system_init(&system, world, builder, true, entity_update, NULL, NULL);

    ecs_system_update(&system, 0);
    ck_assert_msg(*c1 && *c2 && *c3, "EcsEntity system failed to update for all entities");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(entity_enabled_update_some) {
    puts("moo 3");
    EcsEntitySetBuilder* builder = ecs_entity_set_builder_init();
    ecs_entity_set_with(builder, bool_component);
    ecs_entity_set_without(builder, int_component);

    EcsWorld world = ecs_world_init();
    EcsEntity entity1 = ecs_create_entity(world);
    EcsEntity entity2 = ecs_create_entity(world);
    EcsEntity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_entity_set(entity1, bool_component);
    bool* c2 = ecs_entity_set(entity2, bool_component);
    bool* c3 = ecs_entity_set(entity3, bool_component);
    ecs_entity_set(entity2, int_component);

    EcsEntitySystem system;
    ecs_entity_system_init(&system, world, builder, true, entity_update, NULL, NULL);

    ecs_system_update(&system, 0);
    ck_assert_msg(*c1 && *c3 && !*c2, "EcsEntity system failed to update for valid entities");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(entity_disabled_update_none) {
    EcsEntitySetBuilder* builder = ecs_entity_set_builder_init();
    ecs_entity_set_with(builder, bool_component);
    ecs_entity_set_without(builder, int_component);

    EcsWorld world = ecs_world_init();
    EcsEntity entity1 = ecs_create_entity(world);
    EcsEntity entity2 = ecs_create_entity(world);
    EcsEntity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_entity_set(entity1, bool_component);
    bool* c2 = ecs_entity_set(entity2, bool_component);
    bool* c3 = ecs_entity_set(entity3, bool_component);

    EcsEntitySystem system;
    ecs_entity_system_init(&system, world, builder, true, entity_update, NULL, NULL);

    ecs_system_disable(&system);

    ecs_system_update(&system, 0);
    ck_assert_msg(!(*c1 || *c2 || *c3), "EcsEntity system updated invalid entities");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

int main(void) {
    int number_failed;

    Suite* s = suite_create("ECS System");
    TCase* tc_system = tcase_create("ECS System");

    tcase_add_checked_fixture(tc_system, system_start, NULL);
    tcase_add_unchecked_fixture(tc_system, system_setup, system_teardown);

    tcase_add_test(tc_system, action_enabled_update_calls);
    tcase_add_test(tc_system, action_disabled_update_not_calls);
    tcase_add_test(tc_system, sequential_enabled_update_calls_enabled_all);
    tcase_add_test(tc_system, sequential_enabled_update_calls_enabled_some);
    tcase_add_test(tc_system, sequential_disabled_update_calls_none);
    tcase_add_test(tc_system, component_enabled_update_calls);
    tcase_add_test(tc_system, component_disabled_update_not_calls);
    // tcase_add_test(tc_system, component_ignores_disabled_entities);
    tcase_add_test(tc_system, entity_enabled_update_all);
    tcase_add_test(tc_system, entity_enabled_update_some);
    tcase_add_test(tc_system, entity_disabled_update_none);

    suite_add_tcase(s, tc_system);

    SRunner* sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}