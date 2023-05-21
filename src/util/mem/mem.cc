#include "util/mem/mem.hpp"

#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>

void gsync::mem::ConfigureMallocForRt() {
    /* Lock all pages to RAM. */
    if (-1 == mlockall(MCL_CURRENT | MCL_FUTURE)) {
        perror("failed to lock memory pages via mlockall()");
    }

    /* Disable heap trimming. */
    if (!mallopt(M_TRIM_THRESHOLD, -1)) {
        perror("failed to set M_TRIM_THRESHOLD option via mallopt()");
    }

    /* Allocate dynamic memory to the process heap (i.e., disable mmap()). */
    if (!mallopt(M_MMAP_MAX, 0)) {
        perror("failed to set M_MMAP_MAX option via mallopt()");
    }
}

void gsync::mem::PrefaultStack() {
    std::array<unsigned char, kMaxStackSize> dummy = {0};
    for (int64_t i = 0; i < kMaxStackSize; i += sysconf(_SC_PAGESIZE)) {
        dummy.at(i) = 1;
    }
}

void gsync::mem::PrefaultHeap() {
    std::unique_ptr<unsigned char[]> dummy(new unsigned char[kMaxHeapSize]);
    if (!dummy) {
        return;
    }

    for (int64_t i = 0; i < kMaxHeapSize; i += sysconf(_SC_PAGESIZE)) {
        dummy[i] = 1;
    }
}

void gsync::mem::ConfigureMemForRt() {
    ConfigureMallocForRt();
    PrefaultStack();
    PrefaultHeap();
}
