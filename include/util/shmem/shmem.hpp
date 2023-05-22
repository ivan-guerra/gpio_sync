#ifndef SHMEM_H_
#define SHMEM_H_

#include <errno.h>
#include <pthread.h>
#include <sys/shm.h>

#include <stdexcept>
#include <string>

namespace gsync {

/**
 * Wrapper for user data stored in shared memory.
 *
 * IpShMemData is a templated wrapper type. The template parameter is a
 * user defined type to be hosted in shared memory. IpShMemData provides an
 * interface for synchronizing access to the data in shared memory using a
 * mutex. The user is responsible for orchestrating sync using the
 * Lock(), TryLock(), and Unlock() methods.
 */
template <typename T>
struct IpShMemData {
    T data;               /**< User data. */
    pthread_mutex_t lock; /**< Mutex for data access synchronization. */

    /* If you are getting false from any of these methods, run perror()
     * immediately afterwards to see why or check \a errno directly. */

    /** Trivial wrapper around \a pthread_mutex_lock().
     *
     * @returns See \a man \a pthread_mutex_lock.
     */
    int Lock() { return (0 == pthread_mutex_lock(&lock)); }

    /** Trivial wrapper around \a pthread_mutex_trylock().
     *
     * @returns See \a man \a pthread_mutex_trylock.
     */
    int TryLock() { return (0 == pthread_mutex_trylock(&lock)); }

    /** Trivial wrapper around \a pthread_mutex_unlock().
     *
     * @returns See \a man \a pthread_mutex_unlock.
     */
    int Unlock() { return (0 == pthread_mutex_unlock(&lock)); }
};

/**
 * Inter-process shared memory utility.
 *
 * IpShMem implements inter-process shared memory management. IpShMem will
 * allocate a shared memory segment with a user specified key for a user defined
 * type that is specified as the template parameter to the class. If the shared
 * memory segment already exists with the specified key, IpShMem will attach
 * to the existing segment. Processes synchronize data access using the mutex
 * provided in the IpShMemData pointer.
 */
template <typename T>
class IpShMem {
   public:
    /**
     * Allocate or fetch the shared memory associated with the parameter key.
     *
     * @param[in] shmkey A unique shared memory key. The key must be a positive
     * integer.
     *
     * @throws std::runtime_error
     */
    explicit IpShMem(int shmkey);

    /** Detach from/deallocate shared memory. */
    ~IpShMem() { Cleanup(); }

    /* No reason to copy or move IpShMem objects at this time. */
    IpShMem() = delete;
    IpShMem(const IpShMem<T>&) = delete;
    IpShMem& operator=(const IpShMem<T>&) = delete;
    IpShMem(IpShMem<T>&&) = delete;
    IpShMem& operator=(IpShMem<T>&&) = delete;

    /**
     * Return the shared memory key.
     *
     * @returns Shared memory key for this IpShMem segment. A return value of
     *          0 indicates this object is uninitialized.
     */
    int GetKey() const { return key_; }

    /**
     * Return a pointer to IpShMemData.
     *
     * @returns A pointer to this IpShMem object's IpShMemData object.
     *          \a nullptr is returned if this object has not acquired any
     *          shared memory.
     */
    IpShMemData<T>* GetData() { return data_; }
    const IpShMemData<T>* GetData() const { return data_; }

   private:
    void Cleanup();

    bool is_owner_; /**< Flag indicating whether this IpShMem instance allocated
                       the shared memory. */
    int key_;       /**< Shared memory key (User defined). */
    int id_;        /**< Shared memory ID (System defined). */
    IpShMemData<T>* data_; /**< Shared memory data pointer. */
};

template <typename T>
void IpShMem<T>::Cleanup() {
    if (data_) {
        shmdt(data_);
    }

    if (id_ && is_owner_) {
        /* Mark the shm segment for destruction after the last process
         * detaches. */
        shmctl(id_, IPC_RMID, NULL);
    }

    is_owner_ = false;
    key_ = 0;
    id_ = 0;
    data_ = nullptr;
}

template <typename T>
IpShMem<T>::IpShMem(int shmkey)
    : is_owner_(true), key_(0), id_(0), data_(nullptr) {
    if (shmkey <= 0) {
        throw std::runtime_error("error shmem key must be a positive integer");
    }
    key_ = shmkey;

    /* Allocate shared memory. We use IPC_EXCL to ensure we are creating the
     * shared memory segment. If the segment exists, we will just fetch the
     * existing segment. */
    const int kReadWritePerm = 0666;
    id_ = shmget(key_, sizeof(IpShMemData<T>),
                 IPC_CREAT | IPC_EXCL | kReadWritePerm);
    if (EEXIST == errno) {
        /* The shared memory segment exists, lets just get the existing ID. */
        is_owner_ = false;
        id_ = shmget(key_, sizeof(IpShMemData<T>), IPC_CREAT | kReadWritePerm);
    }
    if (id_ < 0) {
        throw std::runtime_error("failed to retrieve specified shmem id");
    }

    /* Attach the shared memory segment identified by id_ to the process
     * address space. */
    int* shm = reinterpret_cast<int*>(shmat(id_, nullptr, 0));
    if (-1 == *shm) {
        Cleanup();
        throw std::runtime_error("failed to attach to shmem");
    }
    data_ = reinterpret_cast<IpShMemData<T>*>(shm);

    /* The shared memory owner must initialize the mutex. */
    if (is_owner_) {
        /* PTHREAD_PROCESS_SHARED is required for the mutex to be shared across
         * processes. PTHREAD_PRIO_INHERIT allows for the kernel to priority
         * boost a process holding a mutex required by a higher priority process
         * or task. */
        pthread_mutexattr_t mtx_attr;
        pthread_mutexattr_init(&mtx_attr);
        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutexattr_setpshared(&mtx_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setprotocol(&mtx_attr, PTHREAD_PRIO_INHERIT);
        if (pthread_mutex_init(&(data_->lock), &mtx_attr)) {
            Cleanup();
            throw std::runtime_error("failed to initialize mutex");
        }
        pthread_mutexattr_destroy(&mtx_attr);
    }
}

}  // namespace gsync

#endif
