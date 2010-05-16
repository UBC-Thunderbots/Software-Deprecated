#ifndef UTIL_RWLOCK_H
#define UTIL_RWLOCK_H

#include "util/noncopyable.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <pthread.h>

//
// An acquisition of a read-write lock.
//
class rwlock_scoped_acquire : public noncopyable {
	public:
		//
		// Acquires the lock.
		//
		rwlock_scoped_acquire(pthread_rwlock_t *lock, typeof(pthread_rwlock_rdlock) acquire_fn) : lock(lock) {
			if (acquire_fn(lock) != 0) {
				throw std::runtime_error("Cannot acquire rwlock!");
			}
		}

		//
		// Releases the lock.
		//
		~rwlock_scoped_acquire() {
			if (pthread_rwlock_unlock(lock) != 0) {
				std::cerr << "Cannot release rwlock!\n";
				std::abort();
			}
		}

	private:
		pthread_rwlock_t * const lock;
};

#endif

