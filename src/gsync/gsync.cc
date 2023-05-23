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

int main(int argc, char** argv) {
    /* Use the SIGINT signal to trigger program exit. */
    if (-1 == InitAction(SIGINT, 0, ExitHandler)) {
        perror("failed to register SIGINT handler");
        return 1;
    }

    if (argc != 3) {
        std::cerr << "error missing one or more required arguments"
                  << std::endl;
        std::cerr << "usage: gtimer GPIO_OUT SHMEM_KEY" << std::endl;
        return 1;
    }

    bool has_error = false;
    try {
        /* See https://programmador.com/posts/real-time-linux-app-development/
         */
        gsync::mem::ConfigureMemForRt();

        /* Attach to shared memory allocated by the gtimer process. */
        const int kShmemKey = std::stoi(argv[2]);
        gsync::IpShMem<struct timespec> shmem_ctrl(kShmemKey);
        gsync::IpShMemData<struct timespec>* peer_runtime =
            shmem_ctrl.GetData();

        /* Export the GPIO which we will be sending our wakeup signals on. */
        const int kGpioNum = std::stoi(argv[1]);
        gsync::Gpio runtime_gpio(kGpioNum);
        runtime_gpio.Dir(gsync::Gpio::Direction::kOutput);
        runtime_gpio.Val(gsync::Gpio::Value::kLow);

        /* Construct the synchronous wakeup 'calculator'. */
        const int kFrequencyHz = 100;
        const double kCouplingConstant = 0.50;
        gsync::KuramotoSync sync(kFrequencyHz, kCouplingConstant);

        RunEventLoop(sync, runtime_gpio, peer_runtime);
    } catch (const std::runtime_error& e) {
        std::cerr << "error: " << e.what() << std::endl;
        has_error = true;
    } catch (const std::exception& e) {
        std::cerr << "error: invalid argument" << std::endl;
        has_error = true;
    }

    return (has_error) ? 1 : 0;
}
