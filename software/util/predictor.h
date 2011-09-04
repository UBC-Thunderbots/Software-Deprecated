#ifndef UTIL_PREDICTOR_H
#define UTIL_PREDICTOR_H

#include "util/time.h"
#include "util/kalman/kalman.h"
#include <utility>

/**
 * Accumulates data points over time and predicts past, current, and future values and derivatives.
 *
 * \tparam T the type of quantity to predict over.
 */
template<typename T> class Predictor {
	public:
		/**
		 * Constructs a new Predictor.
		 *
		 * \param[in] measure_std the expected standard deviation of the measurement noise.
		 *
		 * \param[in] accel_std the standard deviation of noise equivalent to unknown object acceleration.
		 */
		Predictor(T measure_std, T accel_std, double decay_time_constant);

		/**
		 * Gets the predicted value some length of time into the future (or past).
		 *
		 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
		 *
		 * \param[in] deriv the derivative level to take (\c 0 for position or \c 1 for velocity).
		 *
		 * \param[in] ignore_cache \c true to ignore the lookaside, or \c false to use it.
		 *
		 * \return the value and its standard deviation.
		 */
		std::pair<T, T> value(double delta, unsigned int deriv = 0, bool ignore_cache = false) const __attribute__((warn_unused_result));

		/**
		 * Locks in a timestamp to consider as the current time.
		 *
		 * \param[in] ts the timestamp.
		 */
		void lock_time(const timespec &ts);

		/**
		 * Pushes a new sample into the prediction engine.
		 *
		 * \param[in] value the value to add.
		 *
		 * \param[in] ts the timestamp at which the value was sampled.
		 */
		void add_datum(T value, const timespec &ts);

		/**
		 * Clears the accumulated history of the predictor.
		 * This means that on the next addition of a new datum, the predictor will estimate zero for all derivatives.
		 * Until the addition of a new datum, the predictor will not change its output.
		 * This is useful when coordinate transformation changes lead to large step changes in value, such as when switching field ends.
		 */
		void clear();

	private:
		timespec lock_timestamp;
		Kalman filter;
		std::pair<T, T> zero_value, zero_first_deriv;
};

#endif

