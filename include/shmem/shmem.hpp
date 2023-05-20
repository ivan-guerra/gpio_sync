#ifndef SHMEM_H_
#define SHMEM_H_

#include <errno.h>
#include <pthread.h>
#include <sys/shm.h>

namespace gsync {

/**
 * Wrapper for User data stored in shared memory.
 *
 * IpShMemData is a templated wrapper type. The template parameter is a
 * User defined type to be hosted in shared memory. IpShMemData provides an
 * interface for synchronizing access to the data in shared memory using a
 * mutex. The user is responsible for orchestrating sync using the
 * Lock()/TryLock()/Unlock() API.
 */
template <typename T>
struct IpShMemData {
    T data;               /**< User data. */
    pthread_mutex_t lock; /**< Mutex lock. */

    /* If you are getting false from any of these methods, run perror()
     * immediately afterwards to see why or check errno directly. */
    bool Lock() { return (0 == pthread_mutex_lock(&lock)); }
    bool TryLock() { return (0 == pthread_mutex_trylock(&lock)); }
    bool Unlock() { return (0 == pthread_mutex_unlock(&lock)); }
};

/**
 * Inter-process shared memory utility.
 *
 * IpShMem implements inter-process shared memory management. IpShMem will
 * allocate a shared memory segment with a user specified key for a user defined
 * type that is specified as the template parameter to the class. If the shared
 * memory segment already exists with the specified key, IpShMem will attach
 * to the existing segment. Processes synchronize shmem access using the mutex
 * provided in the IpShMemData pointer that is returned on shmem allocation.
 */
template <typename T>
class IpShMem {
   public:
    IpShMem() : is_owner_(true), key_(0), id_(0), data_(nullptr) {}
    ~IpShMem() = default;

    /* No reason to copy or move IpShMem objects. IpShMem objects are light
     * enough that it doesn't hurt performance much to construct a new object
     * and fetch new/existing shmem via Initialize(). */
    IpShMem(const IpShMem<T>&) = delete;
    IpShMem& operator=(const IpShMem<T>&) = delete;
    IpShMem(IpShMem<T>&&) = delete;
    IpShMem& operator=(IpShMem<T>&&) = delete;

    /**
     * Allocate or fetch the shmem associated with the parameter key.
     *
     * Initialize or fetch a shared memory segment identified by the parameter
     * shared memory key.
     *
     * @param shmkey [in] A unique shared memory key. The key must be a positive
     * integer.
     * @returns A pointer to a IpShMemData<T> object representing the
     * allocated/fetched shared memory. nullptr is returned on error and errno
     * is set.
     */
    IpShMemData<T>* Initialize(int shmkey);

    /**
     * Detach from/deallocate shared memory.
     *
     * @returns True if deallocation succeeded. On error, false is returned and
     * errno is set.
     */
    bool Shutdown();

    /**
     * Get the User specified shared memory key.
     *
     * @returns Shared memory key for this IpShMem segment. A return value of
     *          0 indicates this object is uninitialized.
     */
    int GetKey() const { return key_; }

    /**
     * Return a pointer to IpShMemData.
     *
     * @returns A pointer to this IpShMem object's IpShMemData object.
     *          nullptr is returned if this object is uninitialized.
     */
    IpShMemData<T>* GetData() { return data_; }
    const IpShMemData<T>* GetData() const { return data_; }

   private:
    bool is_owner_; /**< Flag indicating whether this IpShMem instance allocated
                       the shared memory. */
    int key_;       /**< Shared memory key (User defined). */
    int id_;        /**< Shared memory ID (System defined). */
    IpShMemData<T>* data_; /**< Shared memory data pointer. */
};

template <typename T>
IpShMemData<T>* IpShMem<T>::Initialize(int shmkey) {
    /* shmkey must be a positive integer. */
    if (shmkey <= 0) {
        return nullptr;
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
        return nullptr;
    }

    /* Attach the shared memory segment identified by id_ to the process
     * address space. */
    int* shm = reinterpret_cast<int*>(shmat(id_, nullptr, 0));
    if (-1 == *shm) {
        return nullptr;
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
            return nullptr;
        }
        pthread_mutexattr_destroy(&mtx_attr);
    }

    return data_;
}

template <typename T>
bool IpShMem<T>::Shutdown() {
    /* Detach the shm segment. */
    int rc = shmdt(data_);
    if (rc < 0) {
        return false;
    }

    if (is_owner_) {
        /* Mark the shm segment for destruction after the last process detaches.
         */
        rc = shmctl(id_, IPC_RMID, NULL);
        if (rc < 0) {
            return false;
        }
    }

    is_owner_ = false;
    key_ = 0;
    id_ = 0;
    data_ = nullptr;

    return true;
}

}  // namespace gsync

#endif
