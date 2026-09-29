#include <stdio.h>
// Usage: build_test_runner foo.c bar.c > run_all_tests.c
// Open each c file
// Find test function definitions / declarations

static const char *source_fragment_head =
"#include \"run_test.h\"\n"
"int main() {\n"
"    struct test_state state = {.verbose = true};";

static const char *source_fragment_tail =
"    print_summary(&state);\n"
"}\n";

static bool
skip_until(FILE *f, const char *s)
{
    int i = 0;
    int c;
    while (s[i] != '\0' && (c = fgetc(f)) != EOF) {
        if (c == s[i]) {
            i++;
        } else if (c == s[0]) {
            i = 1;
        } else {
            i = 0;
        }
    }
    return s[i] == '\0';
}

static void
skip_rest_of_line(FILE *f)
{
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {
    }
}

static bool
at_newline(FILE *f)
{
    int c = getc(f);
    ungetc(c, f);
    return c == '\n';
}

// f is expected to be at the start of a function declaration
// this function will print that function name to stdout.
static void
print_function_name(FILE *f)
{
    int c;
    while ((c = fgetc(f)) != EOF && c != '(') {
        putchar(c);
    }
}

static void
extract_tests(const char *file_name)
{
    FILE *f = fopen(file_name, "r");
    while (skip_until(f, "\nTEST")) {
        unsigned timeout = 0;
        if (!at_newline(f)) {
            fscanf(f, " TIMEOUT(%u)", &timeout);
        }
        skip_rest_of_line(f);
        printf("    RUN_TEST(&state, \"%s\", ", file_name);
        print_function_name(f);
        printf(", %u)\n", timeout);
    }
    fclose(f);
}

int main(int argc, char *argv[])
{
    puts(source_fragment_head);
    for (int i = 1; i < argc; i++) {
        extract_tests(argv[i]);
    }
    puts(source_fragment_tail);
}
