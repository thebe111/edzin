#include <check.h>

START_TEST(edzin_example_1) {
    ck_assert_int_eq(1, 1);
}
END_TEST

START_TEST(edzin_example_2) {
    ck_assert_int_eq(1, 1);
}
END_TEST

Suite* 
__edzin(void) {
    Suite* s;
    TCase* tc_core;

    s = suite_create("EDZIN");

    tc_core = tcase_create("core");

    tcase_add_test(tc_core, edzin_example_1);
    tcase_add_test(tc_core, edzin_example_2);

    suite_add_tcase(s, tc_core);

    return s;
}
