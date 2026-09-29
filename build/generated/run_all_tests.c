#include "run_test.h"
int main() {
    struct test_state state = {.verbose = true};
    RUN_TEST(&state, "src/Arena.c", arenaInitializeNullptrShouldReturnErrCode, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitializeMallocFailureShouldReturnErrCode, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitializeShouldInitWithGivenCapacity, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitializeShouldAllocateCorrectSize, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitializeWithCapacityGreaterThanMaxCapacityShouldUseMaxCapacity, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitializationShouldSucceed, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitializeShouldUseDefaultCapacity, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitFromStaticBufferShouldErrForNullArena, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitFromStaticBufferShouldErrForNullBuffer, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitFromStaticBufferShouldInitCorrectly, 0)
    RUN_TEST(&state, "src/Arena.c", arenaInitFromStaticBufferShouldNotMalloc, 0)
    RUN_TEST(&state, "src/Arena.c", allocAlignedWithoutBlockAllocNullArena, 0)
    RUN_TEST(&state, "src/Arena.c", allocAlignedWithoutBlockAllocInvalidAlignment, 0)
    RUN_TEST(&state, "src/Arena.c", allocAlignedWithoutBlockAllocNoPaddingRequired, 0)
    RUN_TEST(&state, "src/Arena.c", allocAlignedWithoutBlockAllocRequiredPaddingAdded, 0)
    RUN_TEST(&state, "src/Arena.c", allocAlignedWithoutBlockAllocTooBigWithoutPadding, 0)
    RUN_TEST(&state, "src/Arena.c", allocAlignedWithoutBlockAllocTooBigWithPadding, 0)
    print_summary(&state);
}

