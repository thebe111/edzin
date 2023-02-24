#include "./example.c"
#include "./handlers.c"
#include <check.h>
#include <stdlib.h>

int
main() {
    int fails;
    Suite* s;
    SRunner* r;

    /*
     * Init a runner with a suite
     */
    r = srunner_create(__example());

    /*
     * Add new suites
     */
    srunner_add_suite(r, __handlers());

    srunner_run_all(r, CK_NORMAL);
    fails = srunner_ntests_failed(r);
    srunner_free(r);

    return fails == 0;
}
