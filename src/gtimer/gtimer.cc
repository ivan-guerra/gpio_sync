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
        /* Block until the next event occurs on the line. */
        runtime_gpio.WaitForEdge();

        /* Record the peer's last runtime in shmem. */
        runtime_shmem->Lock();
        clock_gettime(CLOCK_MONOTONIC, &runtime_shmem->data);
        runtime_shmem->Unlock();
    }
}

static void PrintUsage() {
    std::cout << "usage: gtimer [OPTIONS]... GPIO_IN SHMEM_KEY" << std::endl;
    std::cout << "GPIO Signal Time Recorder" << std::endl;
    std::cout << "\t-h, --help\tprint this help page" << std::endl;
    std::cout << "\tGPIO_IN\t\tinput gpio pin number" << std::endl;
    std::cout << "\tSHMEM_KEY\tshared memory key" << std::endl;
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
        std::cerr << "error: missing GPIO_IN" << std::endl;
        return 1;
    }
    if (!argv[optind + 1]) {
        std::cerr << "error: missing SHMEM_KEY" << std::endl;
        return 1;
    }

    /* Use the SIGINT signal to trigger program exit. */
    if (-1 == InitAction(SIGINT, 0, ExitHandler)) {
        perror("failed to register SIGINT handler");
        return 1;
    }

    bool has_error = false;
    try {
        /* See https://programmador.com/posts/real-time-linux-app-development/
         */
        gsync::mem::ConfigureMemForRt();

        /* Allocate shared memory slot for storing our peers' last runtime. */
        gsync::IpShMem<struct timespec> shmem_ctrl(std::stoi(argv[optind + 1]));
        gsync::IpShMemData<struct timespec>* runtime_shmem =
            shmem_ctrl.GetData();

        /* Export the GPIO which we will be checking for rising edge events. */
        gsync::Gpio runtime_gpio(std::stoi(argv[optind]));
        runtime_gpio.Dir(gsync::Gpio::Direction::kInput);
        runtime_gpio.EdgeType(gsync::Gpio::Edge::kRising);

        RunEventLoop(runtime_gpio, runtime_shmem);
    } catch (const std::runtime_error& e) {
        std::cerr << "error: " << e.what() << std::endl;
        has_error = true;
    } catch (const std::logic_error& e) {
        std::cerr << "error: GPIO_IN and SHMEM_KEY must be positive integers"
                  << std::endl;
        has_error = true;
    }
    return (has_error) ? 1 : 0;
}
