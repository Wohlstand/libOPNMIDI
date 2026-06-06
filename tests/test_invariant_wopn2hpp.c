#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

START_TEST(test_sprintf_buffer_overflow)
{
    // Invariant: Buffer reads/writes never exceed the declared length.
    // The program must either truncate or reject oversized argv[1] inputs.
    const char *payloads[] = {
        // Exact exploit: 1021 chars + ".h\0" = 1024, overflows typical 1024-byte buffer
        NULL, // will be filled with 1021 'A's
        NULL, // 10x overflow: 10000 chars
        "valid_short_name" // valid input
    };

    char exploit_exact[1022];
    memset(exploit_exact, 'A', 1021);
    exploit_exact[1021] = '\0';
    payloads[0] = exploit_exact;

    char exploit_large[10001];
    memset(exploit_large, 'B', 10000);
    exploit_large[10000] = '\0';
    payloads[1] = exploit_large;

    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        pid_t pid = fork();
        ck_assert_int_ne(pid, -1);
        if (pid == 0) {
            // Child: exec the wopn2hpp binary with the payload as argv[1]
            // It should not crash with a signal (SIGSEGV, SIGABRT, etc.)
            execlp("./wopn2hpp", "wopn2hpp", payloads[i], NULL);
            _exit(127); // exec failed
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                // Must not die from buffer overflow signals
                ck_assert_msg(sig != 11 && sig != 6,
                    "Payload %d caused signal %d (buffer overflow?)", i, sig);
            }
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

    tcase_add_test(tc_core, test_sprintf_buffer_overflow);
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