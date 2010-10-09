#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include <ctime>
#include <stdexcept>
#include <sigc++/sigc++.h>

namespace {
	/**
	 * Gets the current time into a timespec.
	 *
	 * \param[out] result the location at which to store the current time.
	 */
	void timespec_now(timespec *result) {
		if (clock_gettime(CLOCK_MONOTONIC, result) < 0) {
			throw std::runtime_error("Cannot get monotonic time.");
		}
	}

	/**
	 * Gets the current time into a timespec.
	 *
	 * \param[out] result the location at which to store the current time.
	 */
	void timespec_now(timespec &result) {
		timespec_now(&result);
	}

	/**
	 * Adds a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \param[out] result a location at which to store the value of \p ts1 + \p ts2.
	 */
	void timespec_add(const timespec &ts1, const timespec &ts2, timespec &result) {
		result.tv_sec = ts1.tv_sec + ts2.tv_sec;
		result.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
		if (result.tv_nsec >= 1000000000L) {
			++result.tv_sec;
			result.tv_nsec -= 1000000000L;
		}
	}

	/**
	 * Subtracts a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \param[out] result a location at which to store the value of \p ts1 − \p ts2.
	 */
	void timespec_sub(const timespec &ts1, const timespec &ts2, timespec &result) {
		if (ts1.tv_nsec >= ts2.tv_nsec) {
			result.tv_sec = ts1.tv_sec - ts2.tv_sec;
			result.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
		} else {
			result.tv_sec = ts1.tv_sec - ts2.tv_sec - 1;
			result.tv_nsec = ts1.tv_nsec + 1000000000L - ts2.tv_nsec;
		}
	}

	/**
	 * Compares a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \return a positive value if \p ts1 > \p ts2, a negative value if \p ts1 < \p ts2, or zero if \p ts1 = \p ts2.
	 */
	int timespec_cmp(const timespec &ts1, const timespec &ts2) {
		if (ts1.tv_sec != ts2.tv_sec) {
			return ts1.tv_sec > ts2.tv_sec ? 1 : -1;
		} else if (ts1.tv_nsec != ts2.tv_nsec) {
			return ts1.tv_nsec > ts2.tv_nsec ? 1 : -1;
		} else {
			return 0;
		}
	}

	/**
	 * Converts a timespec to a double-precision count of seconds.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the number of seconds represented by \p ts.
	 */
	double timespec_to_double(const timespec &ts) {
		return ts.tv_sec + ts.tv_nsec / 1000000000.0;
	}

	/**
	 * Converts a timespec to an integer count of milliseconds.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the number of milliseconds represented by \p ts.
	 */
	unsigned int timespec_to_millis(const timespec &ts) {
		return ts.tv_sec * 1000U + ts.tv_nsec / 1000000U;
	}
}

#endif

