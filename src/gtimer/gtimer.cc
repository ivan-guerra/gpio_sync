#include <getopt.h>
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
        try {
            /* Block until the next event occurs on the line. */
            runtime_gpio.WaitForEdge();
        } catch (const std::system_error& e) {
            /* We expect Gpio::WaitForEdge() to be interrupted when the user
             * sends SIGINT to exit the program. libgpiod will throw an
             * exception in this case. We can safely ignore that exception. */
        }

        /* Record the peer's last runtime in shmem. */
        runtime_shmem->Lock();
        clock_gettime(CLOCK_MONOTONIC, &runtime_shmem->data);
        runtime_shmem->Unlock();
    }
}

static void PrintUsage() {
    std::cout << "usage: gtimer [OPTION]... GPIO_DEVNAME GPIO_OFFSET SHMEM_KEY"
              << std::endl;
    std::cout << "GPIO Signal Time Recorder" << std::endl;
    std::cout << "\t-h, --help\tprint this help page" << std::endl;
    std::cout << "\tGPIO_DEVNAME\tspecify input gpio device name" << std::endl;
    std::cout << "\tGPIO_OFFSET\tspecify input gpio offset" << std::endl;
    std::cout << "\tSHMEM_KEY\tspecift shared memory key" << std::endl;
}

int main(int argc, char** argv) {
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };
    int opt = '\0';
    int long_index = 0;
    while (-1 != (opt = getopt_long(argc, argv, "h",
                                    static_cast<struct option*>(long_options),
                                    &long_index))) {
        switch (opt) {
            case 'h':
                PrintUsage();
                return 0;
            case '?':
                return 1;
        }
    }
    if (!argv[optind]) {
        std::cerr << "error: missing GPIO_DEVNAME" << std::endl;
        return 1;
    }
    if (!argv[optind + 1]) {
        std::cerr << "error: missing GPIO_OFFSET" << std::endl;
        return 1;
    }
    if (!argv[optind + 2]) {
        std::cerr << "error: missing SHMEM_KEY" << std::endl;
        return 1;
    }

    /* Use the SIGINT signal to trigger program exit. */
    if (-1 == InitAction(SIGINT, 0, ExitHandler)) {
        perror("failed to register SIGINT handler");
        return 1;
    }

    try {
        /* See https://programmador.com/posts/real-time-linux-app-development/
         */
        gsync::mem::ConfigureMemForRt();

        /* Allocate shared memory slot for storing our peers' last runtime. */
        gsync::IpShMem<struct timespec> shmem_ctrl(std::stoi(argv[optind + 2]));
        gsync::IpShMemData<struct timespec>* runtime_shmem =
            shmem_ctrl.GetData();

        /* Config the GPIO which we will be checking for rising edge events. */
        gsync::Gpio runtime_gpio(argv[optind], std::stoi(argv[optind + 1]));
        runtime_gpio.EdgeType(gsync::Gpio::Edge::kRising);

        RunEventLoop(runtime_gpio, runtime_shmem);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
