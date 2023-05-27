#ifndef SYNC_H_
#define SYNC_H_

#include <time.h>

namespace gsync {

class KuramotoSync {
   public:
    static const int kNumParticipants = 2; /**< Machines in the sync loop. */

    /**
     * Construct a Kuramoto sync object with the specified frequency and
     * coupling constant.
     *
     * @param[in] frequency Frequency in Hertz at which this task runs.
     * @param[in] coupling_constant The coupling constant, K, in the Kuramoto
     * Model.
     *
     * @throws std::runtime_error
     */
    KuramotoSync(int frequency, double coupling_constant);

    /* The default copy/move constructors and assignment methods will work for
     * objects of this class. */
    KuramotoSync() = delete;
    ~KuramotoSync() = default;
    KuramotoSync(const KuramotoSync&) = default;
    KuramotoSync& operator=(const KuramotoSync&) = default;
    KuramotoSync(KuramotoSync&&) = default;
    KuramotoSync& operator=(KuramotoSync&&) = default;

    /** Return the base frequency in Hertz. */
    int Frequency() const { return frequency_; }

    /** Return the coupling constant. */
    double CouplingConstant() const { return coupling_constant_; }

    /**
     * Run the Kuramoto algorithm to compute this participant's new wakeup time.
     *
     * @param[in] actual_wakeup The time when this participant actually wokeup
     * to begin its current cycle.
     * @param[in] peer_wakeup The last wakeup time reported by this
     * participant's peer.
     *
     * @returns The new wakeup time for this participant which would bring him
     * closer to or keep him in sync with his peer.
     */
    timespec ComputeNewWakeup(const timespec& actual_wakeup,
                              const timespec& peer_wakeup) const;

   private:
    static constexpr double kSecToNano = 1e9;
    static constexpr double kPi = 3.141592653589793;

    double ToNano(const timespec& ts) const;
    double NanoToRad(double ns) const;
    double RadToNano(double rad) const;
    void NormalizeTime(timespec& ts) const;

    int frequency_;
    double coupling_constant_;
};

}  // namespace gsync

#endif
