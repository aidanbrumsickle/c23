#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "run_test.h"

#define CHUNK_SIZE 4096
#define DEFAULT_TIMEOUT 10

static inline char *
read_all(int fd)
{
    char *out = nullptr;
    char chunk[CHUNK_SIZE];
    size_t length = 0;
    size_t capacity = 0;
    ssize_t bytes_read = 0;
    while ((bytes_read = read(fd, chunk, CHUNK_SIZE)) != 0) {
        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (length + bytes_read > capacity) {
            capacity = (length + bytes_read) * 2;
            char *resized_out = realloc(out, capacity);
            if (!resized_out) {
                if (out) {
                    free(out);
                }
                return nullptr;
            }
            out = resized_out;
        }
        memcpy(out + length, chunk, bytes_read);
        length += bytes_read;
    }
    if (out) {
        out[length] = '\0';
    }
    return out;
}

static inline bool
file_name_changed(const struct test_state *state, const char *file_name)
{
    return state->prev_file_name != nullptr
        && strcmp(state->prev_file_name, file_name) != 0;
}

static inline void
print_file_summary(const struct test_state *state)
{
    fprintf(stderr, "%s: %u/%u tests run, %u failures\n",
            state->prev_file_name,
            state->file_runs,
            state->file_tests,
            state->file_failures);
}

static inline void
update_file_stats(struct test_state *state)
{
    state->file_tests = 0;
    state->file_runs = 0;
    state->file_failures = 0;
    state->file_count++;
}

static inline bool
should_skip_test(const struct test_state *state)
{
    return state->stop_on_failure && state->total_failures > 0;
}

static inline void
redirect_stderr_to_pipe(int pipefd[static 2])
{
    close(pipefd[0]);
    if (dup2(pipefd[1], STDERR_FILENO) == -1) {
        perror("dup2");
        _exit(EXIT_FAILURE);
    }
    close(pipefd[1]);
}

static inline void
disable_core_dump_on_assert_failure()
{
    struct rlimit no_core_dump = {};
    setrlimit(RLIMIT_CORE, &no_core_dump);
}

static inline int
run_test_in_subprocess(void (*test)(), unsigned timeout, char **proc_out)
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        redirect_stderr_to_pipe(pipefd);
        disable_core_dump_on_assert_failure();
        alarm(timeout);
        test();
        _exit(EXIT_SUCCESS);
    }
    close(pipefd[1]);
    if (proc_out != nullptr) {
        *proc_out = read_all(pipefd[0]);
    }
    close(pipefd[0]);
    int status;
    waitpid(pid, &status, 0);
    return status;
}

static inline bool
should_print_file_name(const struct test_state *state)
{
    return (state->verbose && state->file_runs == 1)
        || (!state->verbose && state->file_failures == 1);
}

static inline bool
test_passed(int status)
{
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static inline bool
test_timed_out(int status)
{
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM;
}

static inline void
print_passing_test(
        const struct test_state *state,
        const char *file_name,
        const char *test_name,
        const char *output)
{
    if (!state->verbose) {
        return;
    }
    if (should_print_file_name(state)) {
        fprintf(stderr, "%s:\n", file_name);
    }
    fprintf(stderr, "\t%s: PASSED\n", test_name);
    if (output) {
        fprintf(stderr, "\t\t%s\n", output);
    }
}

static inline void
print_failing_test(
        const struct test_state *state,
        int status,
        const char *file_name,
        const char *test_name,
        const char *output,
        unsigned timeout)
{
    if (should_print_file_name(state)) {
        fprintf(stderr, "%s:\n", file_name);
    }
    fprintf(stderr, "\t%s: FAILED\n", test_name);
    if (output) {
        fprintf(stderr, "\t\t%s\n", output);
    }
    if (test_timed_out(status)) {
        fprintf(stderr, "\t\tTimed out after %u seconds\n", timeout);
    }
}

void
run_test(
        struct test_state *state,
        const char *file_name,
        const char *test_name,
        void (*test)(),
        unsigned timeout)
{
    if (!state) {
        fprintf(stderr, "NULL Test State\n");
        return;
    }
    timeout = timeout ? timeout : DEFAULT_TIMEOUT;
    if (file_name_changed(state, file_name)) {
        print_file_summary(state);
        update_file_stats(state);
    }
    state->prev_file_name = file_name;
    state->total_tests++;
    state->file_tests++;
    if (should_skip_test(state)) {
        return;
    }
    char *output;
    int status = run_test_in_subprocess(test, timeout, &output);
    state->file_runs++;
    state->total_runs++;
    if (test_passed(status)) {
        print_passing_test(state, file_name, test_name, output);
    } else {
        state->file_failures++;
        state->total_failures++;
        print_failing_test(
                state, status, file_name, test_name, output, timeout);
    }
    if (output) {
        free(output);
    }
}

void
print_summary(struct test_state *state)
{
    if (!state) {
        fprintf(stderr, "NULL Test State\n");
        return;
    }
    if (state->prev_file_name) {
        print_file_summary(state);
        update_file_stats(state);
    }
    fprintf(stderr, "%u/%u tests run in %u files with %u failures\n",
            state->total_runs,
            state->total_tests,
            state->file_count,
            state->total_failures);
}
