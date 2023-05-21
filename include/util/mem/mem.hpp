#ifndef MEM_H_
#define MEM_H_

namespace gsync {
namespace mem {

static const int kMaxStackSize = 512 * 1024; /**< 512kib default stack size */
static const int kMaxHeapSize = 8 * 1024 * 1024; /**< 8Mib default heap size */

/** Lock all pages to memory, disable mmap usage, and disable heap trimming.
 */
void ConfigureMallocForRt();

/** Trigger as many page faults as needed to have a stack of size
 * mem::kMaxStackSize. */
void PrefaultStack();

/** Trigger as many page faults as needed to have a heap of size
 * mem::kMaxHeapSize.
 */
void PrefaultHeap();

/**
 * Make the processes' memory layout real-time friendly.
 *
 * See the "Memory Management" section of
 * https://programmador.com/posts/real-time-linux-app-development/
 * for details.
 */
void ConfigureMemForRt();

}  // namespace mem
}  // namespace gsync

#endif
