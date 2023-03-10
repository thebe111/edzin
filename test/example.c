#include <check.h>

START_TEST(test_example) {
    ck_assert_int_eq(1, 1);
}
END_TEST

Suite* 
__example(void) {
    Suite* s;
    TCase* tc_core;

    /* 
     * insert suite name here
     */
    s = suite_create("example.c");

    tc_core = tcase_create("core");

    /*
     * For each new test case new to produce a new line 'linking' the test wiht the suite core
     */
    tcase_add_test(tc_core, test_example);

    suite_add_tcase(s, tc_core);

    return s;
}
