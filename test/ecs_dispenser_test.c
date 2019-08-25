#include <stdlib.h>

#include "check.h"
#include "ecs.h"

START_TEST(ecs_dispenser_initial_value_default) {
    EcsIntDispenser dispenser;
    ecs_dispenser_init(&dispenser);
    int value = ecs_dispenser_get(&dispenser);
    ck_assert_msg(value == 0, "Int Dispenser starts with an invalid value");
    ecs_dispenser_free_resources(&dispenser);
}
END_TEST

START_TEST(ecs_dispenser_initial_value_custom) {
    EcsIntDispenser dispenser;
    int start = 5;
    ecs_dispenser_init_start(&dispenser, start);
    int value = ecs_dispenser_get(&dispenser);
    ck_assert_msg(value == start, "Int Dispenser starts with an invalid value");
    ecs_dispenser_free_resources(&dispenser);
}
END_TEST

START_TEST(ecs_dispenser_get_valid) {
    EcsIntDispenser dispenser;
    ecs_dispenser_init(&dispenser);
    int first = ecs_dispenser_get(&dispenser);
    int second = ecs_dispenser_get(&dispenser);
    ecs_dispenser_release(&dispenser, first);
    int third = ecs_dispenser_get(&dispenser);
    ck_assert_msg(first == third, "Int Dispenser doesn't return freed int when available");
    ecs_dispenser_free_resources(&dispenser);
}
END_TEST

int main(void) {
    int number_failed;

    Suite* s = suite_create("Int Dispenser");
    TCase* tc_dispenser = tcase_create("Int Dispenser");

    tcase_add_test(tc_dispenser, ecs_dispenser_initial_value_default);
    tcase_add_test(tc_dispenser, ecs_dispenser_initial_value_custom);
    tcase_add_test(tc_dispenser, ecs_dispenser_get_valid);

    suite_add_tcase(s, tc_dispenser);

    SRunner* sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}