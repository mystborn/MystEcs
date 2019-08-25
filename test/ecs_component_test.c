#include <stdlib.h>
#include <stdint.h>

#include <stdio.h>

#include "check.h"
#include "ecs.h"

int test_number = 0;
int numbers_created = 0;
int numbers_freed = 0;

EcsWorld world;
ComponentManager* number_component;
ComponentManager* number_pointer_component;

void number_pointer_constructor(void* item) {
    numbers_created++;
    int** ptr = item;
    *ptr = ecs_malloc(sizeof(int));
}

void number_pointer_destructor(void* item) {
    numbers_freed++;
    ecs_free(*(int**)item);
}

void component_setup(void) {
    ecs_init();
    number_component = ecs_component_define(sizeof(int), NULL, NULL);
    number_pointer_component = ecs_component_define(sizeof(int*), number_pointer_constructor, number_pointer_destructor);
}

void component_teardown(void) {
    ecs_component_free(number_component);
    ecs_component_free(number_pointer_component);
}

void component_start(void) {
    world = ecs_world_init();
}

void component_restart(void) {
    ecs_world_free(world);
    numbers_created = 0;
    numbers_freed = 0;
}

START_TEST(component_allocates_correctly) {
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    int* number1 = ecs_component_set(entity1, number_component);
    int* number2 = ecs_component_set(entity2, number_component);
    ptrdiff_t offset = number2 - number1;
    ck_assert_msg(offset == 1, "Components are not allocated next to each other.");
    ck_assert(ecs_component_remove(entity1, number_component) == ECS_RESULT_SUCCESS);
    ck_assert(ecs_component_remove(entity2, number_component) == ECS_RESULT_SUCCESS);
    ecs_entity_free(entity1);
    ecs_entity_free(entity2);
}
END_TEST

START_TEST(component_calls_constructor_and_destructor) {
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    int** ptr1 = ecs_component_set(entity1, number_pointer_component);
    int** ptr2 = ecs_component_set(entity2, number_pointer_component);
    ck_assert_msg(numbers_created == 2, "Did not call constructor");
    ecs_component_remove(entity1, number_pointer_component);
    ecs_component_remove(entity2, number_pointer_component);
    ck_assert_msg(numbers_freed == 2, "Did not call destructor");
    ecs_entity_free(entity1);
    ecs_entity_free(entity2);
}
END_TEST

START_TEST(component_exists_valid_component) {
    Entity entity1 = ecs_create_entity(world);
    ecs_component_set(entity1, number_component);
    ck_assert_msg(ecs_component_exists(entity1, number_component), "Valid component does not exist");
    ecs_entity_free(entity1);

}
END_TEST

START_TEST(component_exists_invalid_component) {
    Entity entity1 = ecs_create_entity(world);
    ck_assert_msg(!ecs_component_exists(entity1, number_component), "Invalid component exists");
    ecs_entity_free(entity1);
}
END_TEST

START_TEST(component_set_sets_flag) {
    Entity entity1 = ecs_create_entity(world);
    ComponentEnum* components = ecs_entity_get_components(entity1);
    ComponentFlag flag = number_component->flag;
    ecs_component_set(entity1, number_component);
    ck_assert_msg(ecs_component_enum_get_flag(components, flag), "Component set did not set flag");
    ecs_component_remove(entity1, number_component);
    ck_assert_msg(!ecs_component_enum_get_flag(components, flag), "Component remove did not remove flag");
    ecs_entity_free(entity1);
}
END_TEST

START_TEST(component_get_valid_component) {
    Entity entity1 = ecs_create_entity(world);
    int** ptr1 = ecs_component_set(entity1, number_pointer_component);
    int** ptr2 = NULL;
    ck_assert_msg(ecs_component_get(entity1, number_pointer_component, &ptr2) == ECS_RESULT_SUCCESS, "Failed to get valid component");
    ck_assert_msg(*ptr1 == *ptr2, "Component get returned wrong value");
    ecs_entity_free(entity1);
}
END_TEST

START_TEST(component_get_invalid_component) {
    Entity entity1 = ecs_create_entity(world);
    int** value;
    ck_assert_msg(ecs_component_get(entity1, number_component, &value) == ECS_RESULT_INVALID_ENTITY, "Got component for invalid entity");
    ecs_entity_free(entity1);
}
END_TEST

START_TEST(entity_free_removes_components) {
    Entity entity1 = ecs_create_entity(world);
    ecs_component_set(entity1, number_pointer_component);
    ecs_entity_free(entity1);
    ck_assert_msg((numbers_freed == 1), "Did not call destructor on entity free");
}
END_TEST

START_TEST(component_free_destroys_all_components) {
    ComponentManager* manager = ecs_component_define(sizeof(int*), number_pointer_constructor, number_pointer_destructor);
    Entity entity1 = ecs_create_entity(world);
    int** ptr1 = ecs_component_set(entity1, manager);
    ck_assert(numbers_created == 1);
    ecs_component_free(manager);
    ck_assert(numbers_freed == 1);
    ecs_entity_free(entity1);
}
END_TEST

START_TEST(component_get_all) {
    Entity entity1 = ecs_create_entity(world);
    Entity entity2 = ecs_create_entity(world);
    Entity entity3 = ecs_create_entity(world);
    *(int*)ecs_component_set(entity1, number_component) = 0;
    *(int*)ecs_component_set(entity2, number_component) = 1;
    *(int*)ecs_component_set(entity3, number_component) = 2;
    int count;
    int* ints = ecs_component_get_all(world, number_component, &count);
    ck_assert(count == 3);
    for(int i = 0; i < count; ++i)
        ck_assert(ints[i] == i);
    ecs_entity_free(entity1);
    ecs_entity_free(entity2);
    ecs_entity_free(entity3);
}
END_TEST

int main(void) {
    int number_failed;

    Suite* s = suite_create("ECS Components");
    TCase* tc_component = tcase_create("ECS Components");

    tcase_add_checked_fixture(tc_component, component_start, component_restart);
    tcase_add_unchecked_fixture(tc_component, component_setup, component_teardown);

    tcase_add_test(tc_component, component_allocates_correctly);
    tcase_add_test(tc_component, component_calls_constructor_and_destructor);
    tcase_add_test(tc_component, component_exists_valid_component);
    tcase_add_test(tc_component, component_exists_invalid_component);
    tcase_add_test(tc_component, component_set_sets_flag);
    tcase_add_test(tc_component, component_get_valid_component);
    tcase_add_test(tc_component, component_get_invalid_component);
    tcase_add_test(tc_component, entity_free_removes_components);
    tcase_add_test(tc_component, component_free_destroys_all_components);
    tcase_add_test(tc_component, component_get_all);

    suite_add_tcase(s, tc_component);

    SRunner* sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}