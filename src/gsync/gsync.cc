#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <iostream>

#include "sync/sync.hpp"
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

static timespec SetWakeupUsingBaseFreq(timespec& actual_wakeup,
                                       int frequency_hz) {
    const int64_t kSecToNano = 1000000000;
    double dt_nano = (1.0 / frequency_hz) * kSecToNano;

    /* Add the wakeup delta based on the base frequency. */
    timespec new_wakeup = {
        .tv_sec = actual_wakeup.tv_sec,
        .tv_nsec = actual_wakeup.tv_nsec + static_cast<long>(dt_nano),
    };

    /* Normalize the timespec. */
    while (new_wakeup.tv_nsec >= kSecToNano) {
        new_wakeup.tv_sec++;
        new_wakeup.tv_nsec -= kSecToNano;
    }
    return new_wakeup;
}

static void RunEventLoop(const gsync::KuramotoSync& sync,
                         gsync::Gpio& runtime_gpio,
                         gsync::IpShMemData<struct timespec>* peer_runtime) {
    auto TsEqual = [](const timespec& a, const timespec& b) {
        return ((a.tv_sec == b.tv_sec) && (a.tv_nsec == b.tv_nsec));
    };

    timespec empty_ts = {.tv_sec = 0, .tv_nsec = 0};
    timespec expected_wakeup = {};
    timespec actual_wakeup = {};
    timespec peer_wakeup = {};
    timespec new_wakeup = {};
    timespec prev_peer_wakeup = {};

    clock_gettime(CLOCK_MONOTONIC, &expected_wakeup);
    while (!exit_gtimer) {
        /* Send wakeup signal to our peer. */
        runtime_gpio.Val(gsync::Gpio::Value::kHigh);

        /* Record our true wakeup time. */
        clock_gettime(CLOCK_MONOTONIC, &actual_wakeup);

        /* Record our peer's last reported wakeup time. */
        peer_runtime->Lock();
        peer_wakeup = peer_runtime->data;
        peer_runtime->Unlock();

        if (TsEqual(empty_ts, peer_wakeup) ||
            TsEqual(prev_peer_wakeup, peer_wakeup)) {
            /* Our peer is offline or not reporting for some other reason.
             * Schedule wakeup using the base frequency. */
            new_wakeup =
                SetWakeupUsingBaseFreq(actual_wakeup, sync.Frequency());
        } else {
            /* Compute a new wakeup time that will bring us in or keep us in
             * sync with our peer. */
            new_wakeup = sync.ComputeNewWakeup(expected_wakeup, actual_wakeup,
                                               peer_wakeup);
        }
        prev_peer_wakeup = peer_wakeup;

        /* Bring down the GPIO line as we wrap up this run. */
        runtime_gpio.Val(gsync::Gpio::Value::kLow);

        /* Save off our expected wakeup time and sleep until our next cycle. */
        expected_wakeup = new_wakeup;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &new_wakeup, NULL);
    }
}

static void PrintUsage() {
    std::cout << "usage: gsync [OPTIONS]... GPIO_OUT SHMEM_KEY" << std::endl;
    std::cout << "GPIO Based Synchronizer" << std::endl;
    std::cout << "\t-f, --frequency\t\tsync task frequency in Hz" << std::endl;
    std::cout << "\t-k, --coupling-const\tKuramoto coupling constant"
              << std::endl;
    std::cout << "\t-h, --help\t\tprint this help page" << std::endl;
    std::cout << "\tGPIO_OUT\t\toutput gpio pin number" << std::endl;
    std::cout << "\tSHMEM_KEY\t\tshared memory key" << std::endl;
}

int main(int argc, char** argv) {
    const int kDefaultFreqHz = 100;
    const double kDefaultCouplingConst = 0.5;

    struct option long_options[] = {
        {"frequency", required_argument, 0, 'f'},
        {"coupling-const", required_argument, 0, 'k'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };
    int opt = '\0';
    int long_index = 0;
    int frequency_hz = kDefaultFreqHz;
    double coupling_const = kDefaultCouplingConst;
    while (-1 != (opt = getopt_long(argc, argv, "hf:k:",
                                    static_cast<struct option*>(long_options),
                                    &long_index))) {
        switch (opt) {
            case 'f':
                try {
                    frequency_hz = std::stoi(optarg);
                } catch (const std::logic_error& e) {
                    std::cerr << "error: frequency must be a postive integer"
                              << std::endl;
                    return 1;
                }
                break;
            case 'k':
                try {
                    coupling_const = std::stod(optarg);
                } catch (const std::logic_error& e) {
                    std::cerr << "error: coupling constant must be a postive "
                                 "floating point"
                              << std::endl;
                    return 1;
                }
                break;
            case 'h':
                PrintUsage();
                return 0;
            case '?':
                return 1;
        }
    }
    if (!argv[optind]) {
        std::cerr << "error: missing GPIO_OUT" << std::endl;
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

        /* Attach to shared memory allocated by the gtimer process. */
        const int kShmemKey = std::stoi(argv[optind + 1]);
        gsync::IpShMem<struct timespec> shmem_ctrl(kShmemKey);
        gsync::IpShMemData<struct timespec>* peer_runtime =
            shmem_ctrl.GetData();

        /* Export the GPIO which we will be sending our wakeup signals on. */
        const int kGpioNum = std::stoi(argv[optind]);
        gsync::Gpio runtime_gpio(kGpioNum);
        runtime_gpio.Dir(gsync::Gpio::Direction::kOutput);
        runtime_gpio.Val(gsync::Gpio::Value::kLow);

        /* Construct the synchronous wakeup 'calculator'. */
        gsync::KuramotoSync sync(frequency_hz, coupling_const);

        RunEventLoop(sync, runtime_gpio, peer_runtime);
    } catch (const std::runtime_error& e) {
        std::cerr << "error: " << e.what() << std::endl;
        has_error = true;
    } catch (const std::logic_error& e) {
        std::cerr << "error: GPIO_OUT and SHMEM_KEY must be positive integers"
                  << std::endl;
        has_error = true;
    }
    return (has_error) ? 1 : 0;
}
