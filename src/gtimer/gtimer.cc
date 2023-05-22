#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <iostream>

#include "util/gpio/gpio.hpp"
#include "util/mem/mem.hpp"
#include "util/shmem/shmem.hpp"

/* An atomic_bool used within a signal handler context must be lock free. */
static_assert(std::atomic<bool>::is_always_lock_free);
std::atomic_bool exit_gtimer = false;

static void ExitHandler(int sig) {
    (void)sig; /* Cast to void to avoid unused variable warning. */
    exit_gtimer = true;
}

static int InitAction(int sig, int flags, void (*handler)(int)) {
    struct sigaction action {};
    action.sa_flags = flags;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    return sigaction(sig, &action, NULL);
}

/* Wait for rising edge events on the GPIO. When an event comes, log the
 * CLOCK_MONOTONIC time in shared memory. */
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

    bool has_error = false;
    try {
        /* See https://programmador.com/posts/real-time-linux-app-development/
         */
        gsync::mem::ConfigureMemForRt();

        /* Allocate shared memory slot for storing our peers' last runtime. */
        const int kShmemKey = std::stoi(argv[2]);
        gsync::IpShMem<struct timespec> shmem_ctrl(kShmemKey);
        gsync::IpShMemData<struct timespec>* runtime_shmem =
            shmem_ctrl.GetData();

        /* Export the GPIO which we will be polling for rising edge events. */
        const int kGpioNum = std::stoi(argv[1]);
        gsync::Gpio runtime_gpio(kGpioNum);
        runtime_gpio.Dir(gsync::Gpio::Direction::kInput);
        runtime_gpio.EdgeType(gsync::Gpio::Edge::kRising);

        RunEventLoop(runtime_gpio, runtime_shmem);
    } catch (const std::runtime_error& e) {
        std::cerr << "error: " << e.what() << std::endl;
        has_error = true;
    } catch (const std::exception& e) {
        std::cerr << "error: invalid argument" << std::endl;
        has_error = true;
    }

    return (has_error) ? 1 : 0;
}
