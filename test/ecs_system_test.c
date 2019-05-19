#include "check.h"
#include "ecs.h"

ComponentManager* bool_component;
ComponentManager* int_component;

int action_count;

void action_update(EcsActionSystem* system, float state) {
    action_count++;
}

void component_update(EcsComponentSystem* system, float state, void* item) {
    *(bool*)item = true;
}

void entity_update(EcsEntitySystem* system, float state, Entity entity) {
    bool* item;
    ecs_component_get(entity, bool_component, &item);
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
    ecs_system_update(&system, 0);
    ck_assert_msg(action_count == 1, "Action system did not update when enabled");
    ecs_system_free_resources(&system);
}
END_TEST

START_TEST(action_disabled_update_not_calls) {
    EcsActionSystem system;
    ecs_action_system_init(&system, action_update, NULL, NULL);
    ecs_system_disable(&system);
    ecs_system_update(&system, 0);
    ck_assert_msg(action_count == 0, "Action system updated when disabled");
    ecs_system_free_resources(&system);
}
END_TEST

START_TEST(sequential_enabled_update_calls_enabled_all) {
    EcsActionSystem a1, a2;
    EcsSequentialSystem system;

    ecs_action_system_init(&a1, action_update, NULL, NULL);
    ecs_action_system_init(&a2, action_update, NULL, NULL);

    ecs_sequential_system_init(&system, NULL, NULL, false, 2, &a1, &a2);
    ecs_system_update(&system, 0);

    ck_assert_msg(action_count == 2, "Sequential system failed to call all children");
    ecs_system_free_resources(&a1);
    ecs_system_free_resources(&a2);
    ecs_system_free_resources(&system);
}
END_TEST

START_TEST(sequential_enabled_update_calls_enabled_some) {
    EcsActionSystem a1, a2;
    EcsSequentialSystem system;

    ecs_action_system_init(&a1, action_update, NULL, NULL);
    ecs_action_system_init(&a2, action_update, NULL, NULL);
    ecs_system_disable(&a1);

    ecs_sequential_system_init(&system, NULL, NULL, false, 2, &a1, &a2);
    ecs_system_update(&system, 0);

    ck_assert_msg(action_count == 1, "Sequential system failed to call all children");
    ecs_system_free_resources(&a1);
    ecs_system_free_resources(&a2);
    ecs_system_free_resources(&system);
}
END_TEST

START_TEST(sequential_disabled_update_calls_none) {
    EcsActionSystem a1, a2;
    EcsSequentialSystem system;

    ecs_action_system_init(&a1, action_update, NULL, NULL);
    ecs_action_system_init(&a2, action_update, NULL, NULL);

    ecs_sequential_system_init(&system, NULL, NULL, false, 2, &a1, &a2);
    ecs_system_disable(&system);

    ecs_system_update(&system, 0);

    ck_assert_msg(action_count == 0, "Sequential system updated when disabled");
    ecs_system_free_resources(&a1);
    ecs_system_free_resources(&a2);
    ecs_system_free_resources(&system);
}
END_TEST

START_TEST(component_enabled_update_calls) {
    EcsWorld world = ecs_world_init();
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    Entity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_component_set(entity1, bool_component);
    bool* c2 = ecs_component_set(entity2, bool_component);
    bool* c3 = ecs_component_set(entity3, bool_component);
    EcsComponentSystem system;
    ecs_component_system_init(&system, world, bool_component, component_update, NULL, NULL);
    ecs_system_update(&system, 0);
    ck_assert_msg(*c1 && *c2 && *c3, "Component system failed to update for all components");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(component_disabled_update_not_calls) {
    EcsWorld world = ecs_world_init();
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    Entity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_component_set(entity1, bool_component);
    bool* c2 = ecs_component_set(entity2, bool_component);
    bool* c3 = ecs_component_set(entity3, bool_component);
    EcsComponentSystem system;
    ecs_component_system_init(&system, world, bool_component, component_update, NULL, NULL);
    ecs_system_disable(&system);
    ecs_system_update(&system, 0);
    ck_assert_msg(!(*c1 || *c2 || *c3), "Component system updated components when disabled");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(entity_enabled_update_all) {
    ComponentEnum with = COMPONENT_ENUM_DEFAULT;
    ComponentEnum without = COMPONENT_ENUM_DEFAULT;
    ecs_component_enum_set_flag(&with, bool_component->flag, true);
    ecs_component_enum_set_flag(&without, int_component->flag, true);

    EcsWorld world = ecs_world_init();
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    Entity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_component_set(entity1, bool_component);
    bool* c2 = ecs_component_set(entity2, bool_component);
    bool* c3 = ecs_component_set(entity3, bool_component);

    EcsEntitySystem system;
    ecs_entity_system_init(&system, world, &with, &without, entity_update, NULL, NULL);

    ecs_component_enum_free_resources(&with);
    ecs_component_enum_free_resources(&without);

    ecs_system_update(&system, 0);
    ck_assert_msg(*c1 && *c2 && *c3, "Entity system failed to update for all entities");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(entity_enabled_update_some) {
    ComponentEnum with = COMPONENT_ENUM_DEFAULT;
    ComponentEnum without = COMPONENT_ENUM_DEFAULT;
    ecs_component_enum_set_flag(&with, bool_component->flag, true);
    ecs_component_enum_set_flag(&without, int_component->flag, true);

    EcsWorld world = ecs_world_init();
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    Entity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_component_set(entity1, bool_component);
    bool* c2 = ecs_component_set(entity2, bool_component);
    bool* c3 = ecs_component_set(entity3, bool_component);
    ecs_component_set(entity2, int_component);

    EcsEntitySystem system;
    ecs_entity_system_init(&system, world, &with, &without, entity_update, NULL, NULL);

    ecs_component_enum_free_resources(&with);
    ecs_component_enum_free_resources(&without);

    ecs_system_update(&system, 0);
    ck_assert_msg(*c1 && *c3 && !*c2, "Entity system failed to update for valid entities");
    ecs_system_free_resources(&system);
    ecs_world_free(world);
}
END_TEST

START_TEST(entity_disabled_update_none) {
    ComponentEnum with = COMPONENT_ENUM_DEFAULT;
    ComponentEnum without = COMPONENT_ENUM_DEFAULT;
    ecs_component_enum_set_flag(&with, bool_component->flag, true);
    ecs_component_enum_set_flag(&without, int_component->flag, true);

    EcsWorld world = ecs_world_init();
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    Entity entity3 = ecs_create_entity(world);
    bool* c1 = ecs_component_set(entity1, bool_component);
    bool* c2 = ecs_component_set(entity2, bool_component);
    bool* c3 = ecs_component_set(entity3, bool_component);

    EcsEntitySystem system;
    ecs_entity_system_init(&system, world, &with, &without, entity_update, NULL, NULL);

    ecs_system_disable(&system);

    ecs_component_enum_free_resources(&with);
    ecs_component_enum_free_resources(&without);

    ecs_system_update(&system, 0);
    ck_assert_msg(!(*c1 || *c2 || *c3), "Entity system updated invalid entities");
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