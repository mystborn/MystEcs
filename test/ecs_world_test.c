#include <stdlib.h>
#include <stdio.h>

#include "check.h"
#include "ecs.h"

static bool value = false;
static int ecs_test_number = 0;
EcsEventManager* bool_event;

void set_value(bool result) {
    value = result;
}

void world_setup(void) {
    ecs_init();
    bool_event = ecs_event_define();
}

void world_teardown(void) {
    ecs_event_manager_free(bool_event);
}

void world_reset(void) {
    printf("Test Number %d Completed\n", ecs_test_number++);
    value = false;
}

START_TEST(world_free_should_free) {
    EcsWorld world = ecs_world_init();
    ck_assert_msg(world != 0, "Invalid world id");
    ck_assert_msg(ecs_world_free(world) == ECS_RESULT_SUCCESS, "Unable to free world");
}
END_TEST

START_TEST(world_free_should_not_free) {
    EcsWorld world = 47;
    ck_assert_msg(ecs_world_free(world) == ECS_RESULT_INVALID_WORLD, "Freed invalid world");
    world = -1;
    ck_assert_msg(ecs_world_free(world) == ECS_RESULT_INVALID_WORLD, "Freed invalid world");
}
END_TEST

START_TEST(subscribe_should_subscribe) {
    EcsWorld world = ecs_world_init();

    int id = ecs_event_subscribe(world, bool_event, set_value);

    ecs_event_publish(world, bool_event, void (*)(bool), true);

    ck_assert(value == true);

    ecs_event_unsubscribe(world, bool_event, id);
    ecs_world_free(world);
}
END_TEST

START_TEST(subscribe_dispose_should_unsubscribe) {
    EcsWorld world = ecs_world_init();

    ecs_event_subscribe(world, bool_event, set_value);

    ecs_event_publish(world, bool_event, void (*)(bool), true);

    ck_assert(value == true);

    value = false;
    ecs_world_free(world);

    ecs_event_publish(world, bool_event, void (*)(bool), true);

    ck_assert(value == false);
}
END_TEST

START_TEST(create_entity_returns_an_entity) {
    EcsWorld world = ecs_world_init();

    Entity entity = ecs_create_entity(world);
    ck_assert_msg(entity.world == world, "Entity created on the wrong world");
    ck_assert_msg(ecs_entity_is_alive(entity), "New entity not alive");
    ck_assert_msg(ecs_entity_is_enabled(entity), "New entity not enabled");
    ck_assert_msg(ecs_entity_free(entity) == ECS_RESULT_SUCCESS, "New entity not freed");
    ecs_world_free(world);
}
END_TEST

START_TEST(free_invalid_entity_should_fail) {
    Entity entity = { -2, 0 };
    ck_assert_msg(ecs_entity_free(entity) == ECS_RESULT_INVALID_WORLD, "Entity on invalid world freed");

    EcsWorld world = ecs_world_init();

    entity.world = world;
    entity.id = 47;
    ck_assert_msg(ecs_entity_free(entity) == ECS_RESULT_INVALID_ENTITY, "Invalid entity freed");
    entity.id = -1;
    ck_assert_msg(ecs_entity_free(entity) == ECS_RESULT_INVALID_ENTITY, "Invalid entity freed");
}
END_TEST

START_TEST(entity_can_be_disabled) {
    EcsWorld world = ecs_world_init();
    Entity entity = ecs_create_entity(world);

    ck_assert_msg(ecs_entity_enable(entity) == ECS_RESULT_INVALID_STATE, "Enabled an enabled entity");
    ck_assert_msg(ecs_entity_disable(entity) == ECS_RESULT_SUCCESS, "Could not disable entity");
    ck_assert_msg(ecs_entity_is_alive(entity), "Disabling entity killed it");
    ck_assert_msg(!ecs_entity_is_enabled(entity), "Disabling entity did not set flag");
    ck_assert_msg(ecs_entity_disable(entity) == ECS_RESULT_INVALID_STATE, "Disabled a disabled entity");
    ck_assert_msg(ecs_entity_enable(entity) == ECS_RESULT_SUCCESS, "Could not enable entity");
    ecs_entity_free(entity);
}
END_TEST

int main(void) {
    int number_failed;

    Suite* s = suite_create("ECS World");
    TCase* tc_world = tcase_create("ECS World");

    tcase_add_checked_fixture(tc_world, NULL, world_reset);
    tcase_add_unchecked_fixture(tc_world, world_setup, world_teardown);

    tcase_add_test(tc_world, world_free_should_free);
    tcase_add_test(tc_world, world_free_should_not_free);
    tcase_add_test(tc_world, subscribe_should_subscribe);
    tcase_add_test(tc_world, subscribe_dispose_should_unsubscribe);
    tcase_add_test(tc_world, create_entity_returns_an_entity);
    tcase_add_test(tc_world, free_invalid_entity_should_fail);
    tcase_add_test(tc_world, entity_can_be_disabled);

    suite_add_tcase(s, tc_world);

    SRunner* sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}