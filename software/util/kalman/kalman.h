#ifndef UTIL_KALMAN_KALMAN_H
#define UTIL_KALMAN_KALMAN_H

#include "util/matrix.h"
#include <deque>

class Kalman {
	public:
		Kalman(bool angle, double measure_std, double accel_std);
		void predict(timespec prediction_time, Matrix &state_predict, Matrix &p_predict) const;
		void update(double measurement, timespec measurement_time);
		void add_control(double input, timespec input_time);
		double get_control(timespec control_time) const;

	private:
		struct ControlInput {
			ControlInput(timespec t, double v);

			timespec time;
			double value;
		};

		void predict_step(double timestep, double control, Matrix &state_predict, Matrix &p_predict) const;
		timespec last_measurement_time;
		double last_control;
		double sigma_m;
		double sigma_a;
		std::deque<ControlInput> inputs;
		Matrix gen_f_mat(double timestep) const;
		Matrix gen_q_mat(double timestep) const;
		Matrix gen_g_mat(double timestep) const;
		Matrix h;
		Matrix p;
		Matrix state_estimate;
		bool is_angle;
};

#endif

