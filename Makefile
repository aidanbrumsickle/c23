CFLAGS := --std=c23 -Wall -Wextra -Werror -g
CPPFLAGS := -MMD -MP -Iheaders

build := build
tools := tools
lib_src := $(wildcard src/*.c)
release_objs := $(lib_src:src/%.c=$(build)/release/%.o)
test_objs := $(lib_src:src/%.c=$(build)/test/%.o)
tool_src := $(wildcard $(tools)/src/*.c)
tool_objs := $(tool_src:$(tools)/src/%.c=$(build)/tools/%.o)
codegen := $(build)/tools/build_test_runner
lib := $(build)/release/libmylib.a

$(lib): $(release_objs)
	$(AR) rcs $@ $^

$(build)/release/%.o: src/%.c | $(build)/release
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(build)/test/%.o: private CPPFLAGS += -Itools/headers
$(build)/test/%.o: src/%.c | $(build)/test
	$(COMPILE.c) -DUNIT_TEST $(OUTPUT_OPTION) $<
$(build)/test/%.o: $(build)/generated/%.c | $(build)/test
	$(COMPILE.c) $(OUTPUT_OPTION) $<


$(build)/tools/%: CPPFLAGS += -Itools/headers
$(build)/tools/%.o: $(tools)/src/%.c | $(build)/tools
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(build)/generated/run_all_tests.c: $(codegen) $(lib_src) | $(build)/generated
	$(codegen) $(lib_src) > $@

$(codegen): $(build)/tools/build_test_runner.o
	$(LINK.o) $^ -o $@

$(build)/tools/run_all_tests: $(test_objs) $(build)/test/run_all_tests.o $(build)/tools/run_test.o $(build)/tools/TestMalloc.o
	$(LINK.o) $^ -o $@

$(build)/release $(build)/test $(build)/tools $(build)/generated:
	mkdir -p $@

.PHONY: all test clean

all: $(lib)

test: $(build)/tools/run_all_tests
	./$(build)/tools/run_all_tests

clean:
	-rm -rf $(build)

-include $(wildcard $(build)/*/*.d)
