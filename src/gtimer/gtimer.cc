#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <iostream>

#include "util/gpio/gpio.hpp"
#include "util/mem/mem.hpp"
#include "util/shmem/shmem.hpp"

/* Make sure the atomic type we'll operate on is lock-free. */
static_assert(std::atomic<bool>::is_always_lock_free);

std::atomic_bool exit_gtimer = false;

static void ExitHandler(int sig) {
    (void)sig; /* Cast to void to avoid unused variable warning. */
    exit_gtimer = true;
}

static int InitAction(int sig, int flags, void (*handler)(int)) {
    struct sigaction action;
    action.sa_flags = flags;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    return sigaction(sig, &action, NULL);
}

static void RunEventLoop(gsync::Gpio& runtime_gpio,
                         gsync::IpShMemData<struct timespec>* runtime_shmem) {
    while (!exit_gtimer) {
        /* Block until the next event occurs on the line. */
        runtime_gpio.WaitForEdge();

        /* Record the peer's last runtime in shmem. */
        runtime_shmem->Lock();
        clock_gettime(CLOCK_MONOTONIC, &runtime_shmem->data);
        runtime_shmem->Unlock();
    }
}

int main(int argc, char** argv) {
    /* See https://programmador.com/posts/real-time-linux-app-development/ */
    gsync::mem::ConfigureMemForRt();

    /* Use the SIGINT signal to trigger program exit. */
    if (-1 == InitAction(SIGINT, 0, ExitHandler)) {
        perror("failed to register SIGINT handler");
        return 1;
    }

    if (argc != 3) {
        std::cerr << "error missing one or more required arguments"
                  << std::endl;
        std::cerr << "usage: gtimer GPIO_NUM SHMEM_KEY" << std::endl;
        return 1;
    }

    /* Allocate shared memory slot for storing our peers' last runtime. */
    const int kShmemKey = std::stoi(argv[2]);
    gsync::IpShMem<struct timespec> shmem_ctrl;
    gsync::IpShMemData<struct timespec>* runtime_shmem =
        shmem_ctrl.Initialize(kShmemKey);
    if (!runtime_shmem) {
        perror("failed to allocate shared memory");
        return 1;
    }

    /* Export the GPIO which we will be polling for rising edge events. */
    const int kGpioNum = std::stoi(argv[1]);
    gsync::Gpio runtime_gpio(kGpioNum);
    if (!runtime_gpio.Dir(gsync::Gpio::Direction::kInput)) {
        perror("failed to set gpio direction to 'in'");
        return 1;
    }
    if (!runtime_gpio.EdgeType(gsync::Gpio::Edge::kRising)) {
        perror("failed to set gpio edge type to 'rising'");
        return 1;
    }

    /* Wait for rising edge events on the GPIO. When an event comes, log the
     * CLOCK_MONOTONIC time in shared memory. */
    RunEventLoop(runtime_gpio, runtime_shmem);

    if (!shmem_ctrl.Shutdown()) {
        perror("failed to cleanup shared memory");
        return 1;
    }

    return 0;
}
