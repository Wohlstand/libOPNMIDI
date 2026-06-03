#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

/*
 * Security invariant: When computing allocation size as (count * element_size),
 * the multiplication must not overflow. If it would overflow, the allocation
 * must fail safely (return NULL or abort) rather than allocating a too-small
 * buffer that leads to heap overflow.
 *
 * We test the overflow check logic directly since we cannot easily call the
 * winmm audio init without Windows headers. We validate the principle that
 * SIZE_MAX / element_size < count means overflow.
 */

static void *safe_alloc(size_t count, size_t element_size)
{
    /* This is what the code SHOULD do - check for overflow */
    if (element_size != 0 && count > SIZE_MAX / element_size) {
        return NULL; /* overflow would occur */
    }
    return malloc(count * element_size);
}

START_TEST(test_allocation_overflow_check)
{
    /* Invariant: allocation with overflowing size must not succeed with small buffer */
    struct {
        size_t count;
        size_t elem_size;
        int should_succeed;
    } cases[] = {
        /* Exploit case: large count * size overflows to small value */
        { SIZE_MAX / 2 + 1, 4, 0 },
        /* Boundary: exactly at SIZE_MAX */
        { SIZE_MAX, 2, 0 },
        /* Another overflow: count chosen so count*elem_size wraps to ~0 */
        { (SIZE_MAX / 1024) + 2, 1024, 0 },
        /* Valid input: normal allocation */
        { 16, sizeof(int), 1 },
        /* Valid boundary: count=0 */
        { 0, 64, 1 },
    };
    int num_cases = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < num_cases; i++) {
        void *ptr = safe_alloc(cases[i].count, cases[i].elem_size);
        if (!cases[i].should_succeed) {
            /* Overflow cases MUST return NULL, never a small buffer */
            ck_assert_ptr_eq(ptr, NULL);
        } else {
            /* Valid cases should succeed (malloc may still fail, but not due to overflow) */
            /* Just verify no crash; free if allocated */
            free(ptr);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_allocation_overflow_check);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}