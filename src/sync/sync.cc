#include "sync/sync.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace gsync {

double KuramotoSync::ToNano(const timespec& ts) const {
    double sec_to_nano = static_cast<double>(ts.tv_sec) * kSecToNano;
    return (sec_to_nano + static_cast<double>(ts.tv_nsec));
}

double KuramotoSync::NanoToRad(double ns) const {
    static const double kConvFactor = (2 * kPi * frequency_) / kSecToNano;
    return (kConvFactor * ns);
}

double KuramotoSync::RadToNano(double rad) const {
    static const double kConvFactor = (kSecToNano / (2 * kPi * frequency_));
    return (kConvFactor * rad);
}

void KuramotoSync::NormalizeTime(timespec& ts) const {
    static const int kNanoSecPerSec = 1e9;
    while (ts.tv_nsec >= kNanoSecPerSec) {
        ts.tv_sec++;
        ts.tv_nsec -= kNanoSecPerSec;
    }
}

KuramotoSync::KuramotoSync(int frequency, double coupling_constant)
    : frequency_(frequency), coupling_constant_(coupling_constant) {
    if (frequency_ <= 0) {
        throw std::runtime_error("frequency must be greater than 0");
    }
    if (coupling_constant_ <= 0.0) {
        throw std::runtime_error("coupling constant must be greater than 0");
    }
}

timespec KuramotoSync::ComputeNewWakeup(const timespec& expected_wakeup,
                                        const timespec& actual_wakeup,
                                        const timespec& peer_wakeup) const {
    double expected_wakeup_ns = ToNano(expected_wakeup);
    double actual_wakeup_ns = ToNano(actual_wakeup);
    double peer_wakeup_ns = ToNano(peer_wakeup);

    /* Compute the variables of the Kuramoto Model for the current run. */
    double omega_i = NanoToRad((1.0 / frequency_) * kSecToNano);
    double dt_i = expected_wakeup_ns - actual_wakeup_ns;
    double dtheta_i = NanoToRad(dt_i);
    double dt_j = expected_wakeup_ns - peer_wakeup_ns;
    double dtheta_j = NanoToRad(dt_j);

    /* Straightforward implementation of the common form of the Kuramoto Model
     * as seen here https://en.wikipedia.org/wiki/Kuramoto_model */
    double dtheta_dt =
        omega_i +
        ((coupling_constant_ / static_cast<double>(kNumParticipants)) *
         std::sin(dtheta_j - dtheta_i));

    /* The new wakeup time is an offset from the actual wakeup time. */
    timespec new_wakeup = {
        .tv_sec = actual_wakeup.tv_sec,
        .tv_nsec =
            actual_wakeup.tv_nsec + static_cast<long>(RadToNano(dtheta_dt)),
    };
    NormalizeTime(new_wakeup);

    return new_wakeup;
}  // namespace gsync

}  // namespace gsync
