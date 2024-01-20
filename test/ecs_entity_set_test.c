#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include "ecs.h"

static EcsComponentManager* int_component;
static EcsComponentManager* bool_component;

static EcsWorld world;

void builder_setup(void) {
    ecs_init();
    int_component = ecs_component_define(sizeof(int), NULL, NULL);
    bool_component = ecs_component_define(sizeof(bool), NULL, NULL);
}

void builder_teardown(void) {
    ecs_component_free(int_component);
    ecs_component_free(bool_component);
}

void builder_start(void) {
    world = ecs_world_init();
}

void builder_stop(void) {
    ecs_world_free(world);
}

START_TEST(set_with_no_constaints_returns_all_entities) {
    int count = 3;
    EcsEntitySetBuilder* builder = ecs_entity_set_builder_init();
    EcsEntitySet* set = ecs_entity_set_build(builder, world, true);
    for(int i = 0; i < count; i++)
        ecs_create_entity(world);
    int set_count;
    ecs_entity_set_get_entities(set, &set_count);
    ck_assert_msg(set_count == count, "Not all entities were added to the set");
    ecs_entity_set_free(set);
}
END_TEST

START_TEST(set_with_component_returns_all_entities_with_component) {
    int count = 4;

    EcsEntitySetBuilder* builder = ecs_entity_set_builder_init();
    ecs_entity_set_with(builder, bool_component);
    EcsEntitySet* set = ecs_entity_set_build(builder, world, true);

    EcsEntity entities[4];
    for(int i = 0; i < count; i++)
        entities[i] = ecs_create_entity(world);

    int set_count;
    ecs_entity_set_get_entities(set, &set_count);
    ck_assert_msg(set_count == 0, "Set filled with invalid entities");

    for(int i = 0; i < count; i++)
        ecs_entity_set(entities[i], bool_component);

    EcsEntity* result =  ecs_entity_set_get_entities(set, &set_count);
    ck_assert_msg(set_count == count, "Set missing valid entities");

    for (int i = 0; i < set_count; i++)
        ck_assert(ecs_entity_exists(result[i], bool_component));

    ecs_entity_remove(entities[2], bool_component);
    result = ecs_entity_set_get_entities(set, &set_count);

    ck_assert_msg(set_count == count - 1, "Set contains invalid entities");

    for (int i = 0; i < set_count; i++)
        ck_assert(ecs_entity_exists(result[i], bool_component));

    ecs_entity_set_free(set);
}
END_TEST

START_TEST(set_without_component_returns_all_entities_without_component) {
    int count = 4;
    
    EcsEntitySetBuilder* builder = ecs_entity_set_builder_init();
    ecs_entity_set_without(builder, int_component);
    EcsEntitySet* set = ecs_entity_set_build(builder, world, true);

    EcsEntity entities[4];
    for(int i = 0; i < count; i++)
        entities[i] = ecs_create_entity(world);

    int set_count;
    EcsEntity* result = ecs_entity_set_get_entities(set, &set_count);
    ck_assert_msg(set_count == count, "Set missing valid entities");

    for (int i = 0; i < set_count; i++)
        ck_assert(!ecs_entity_exists(result[i], int_component));

    for(int i = 0; i < count; i++)
        ecs_entity_set(entities[i], int_component);
    result = ecs_entity_set_get_entities(set, &set_count);
    ck_assert_msg(set_count == 0, "Set filled with invalid entities");

    for (int i = 0; i < set_count; i++)
        ck_assert(!ecs_entity_exists(result[i], int_component));

    ecs_entity_remove(entities[2], int_component);
    result = ecs_entity_set_get_entities(set, &set_count);

    ck_assert_msg(set_count == 1, "Set does not contain all valid entities");

    for (int i = 0; i < set_count; i++)
        ck_assert(!ecs_entity_exists(result[i], int_component));

    ecs_entity_set_free(set);
}
END_TEST

START_TEST(set_includes_previously_created_entity) {
    int count = 4;
    for(int i = 0; i < count; i++)
        ecs_create_entity(world);

    EcsEntitySet* set = ecs_entity_set_build(ecs_entity_set_builder_init(), world, true);
    int set_count;
    ecs_entity_set_get_entities(set, &set_count);
    ck_assert_msg(set_count == count, "Set did not include previously created entities");

    ecs_entity_set_free(set);
}
END_TEST

START_TEST(set_should_not_include_disabled_entity) {
    EcsEntitySet* set = ecs_entity_set_build(ecs_entity_set_builder_init(), world, true);

    int count = 4;
    EcsEntity entities[4];
    for(int i = 0; i < count; i++)
        entities[i] = ecs_create_entity(world);

    ecs_entity_disable(entities[2]);

    int set_count;
    ecs_entity_set_get_entities(set, &set_count);

    ck_assert_msg(set_count == 3, "Set included disabled entity");

    ecs_entity_set_free(set);
}
END_TEST

int main(void) {
    int number_failed;

    Suite* s = suite_create("EcsEntity Set");
    TCase* tc_eb = tcase_create("EcsEntity Set");

    tcase_add_unchecked_fixture(tc_eb, builder_setup, builder_teardown);
    tcase_add_checked_fixture(tc_eb, builder_start, builder_stop);

    tcase_add_test(tc_eb, set_with_no_constaints_returns_all_entities);
    tcase_add_test(tc_eb, set_with_component_returns_all_entities_with_component);
    tcase_add_test(tc_eb, set_without_component_returns_all_entities_without_component);
    tcase_add_test(tc_eb, set_includes_previously_created_entity);
    tcase_add_test(tc_eb, set_should_not_include_disabled_entity);

    suite_add_tcase(s, tc_eb);

    SRunner* sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}