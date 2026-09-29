#define RUN_TEST(pstate, file_name, test, timeout) \
    void test(void); \
    run_test(pstate, file_name, #test, test, timeout);

struct test_state {
    const char *prev_file_name;
    unsigned file_count;
    unsigned file_tests;
    unsigned file_runs;
    unsigned file_failures;
    unsigned total_tests;
    unsigned total_runs;
    unsigned total_failures;
    bool verbose;
    bool stop_on_failure;
};

void
run_test(
        struct test_state *state,
        const char *file_name,
        const char *test_name,
        void (*test)(),
        unsigned timeout);

void
print_summary(struct test_state *state);
