#include "../src/edzin/handlers.h"
#include <check.h>
#include <stdio.h>

START_TEST(handle_arrow_keys_arrow_up) {
#ifdef UO_ENABLE_ARROW_KEYS
    int code = handle_arrow_keys('A');

    ck_assert_int_eq(code, 1002);
#else
    int code = handle_arrow_keys('A');

    ck_assert_int_eq(code, '\x1b');
#endif
}
END_TEST

START_TEST(handle_arrow_keys_arrow_down) {
#ifdef UO_ENABLE_ARROW_KEYS
    int code = handle_arrow_keys('B');

    ck_assert_int_eq(code, 1003);
#else
    int code = handle_arrow_keys('B');

    ck_assert_int_eq(code, '\x1b');
#endif
}
END_TEST

Suite* 
__handlers(void) {
    Suite* s;
    TCase* tc_core;

    s = suite_create("handlers.c");

    tc_core = tcase_create("core");

    tcase_add_test(tc_core, handle_arrow_keys_arrow_up);
    tcase_add_test(tc_core, handle_arrow_keys_arrow_down);

    suite_add_tcase(s, tc_core);

    return s;
}
