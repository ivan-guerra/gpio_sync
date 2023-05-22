#ifndef MEM_H_
#define MEM_H_

#include <string>

namespace gsync {
namespace mem {

static const int kMaxStackSize = 512 * 1024; /**< 512kib default stack size. */
static const int kMaxHeapSize = 8 * 1024 * 1024; /**< 8Mib default heap size. */

/** Lock all process pages in memory, disable \a mmap usage, and disable heap
 * trimming.
 *
 * @throws std::runtime_error
 */
void ConfigureMallocForRt();

/** Trigger as many page faults as needed to have a stack of size
 * mem::kMaxStackSize locked into memory. */
void PrefaultStack();

/** Trigger as many page faults as needed to have a heap of size
 * mem::kMaxHeapSize locked into memory.
 *
 * @throws std::runtime_error
 */
void PrefaultHeap();

/**
 * Make the processes' memory layout real-time friendly.
 *
 * See the "Memory Management" section of
 * https://programmador.com/posts/real-time-linux-app-development/
 * for details.
 *
 * @throws std::runtime_error
 */
void ConfigureMemForRt();

}  // namespace mem
}  // namespace gsync

#endif
