#include "ai/backend/xbee/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace AI::BE::XBee;

namespace {
	const double BATTERY_CRITICAL_THRESHOLD = 13.5;

	const int BATTERY_HYSTERESIS_MAGNITUDE = 15;

	unsigned int calc_kick(double speed) {
		static const double SPEEDS[] = { 7.14, 8.89, 10.3 };
		static const unsigned int POWERS[] = { 2016, 3024, 4032 };

		double speed_below = 0.0, speed_above = 0.0;
		unsigned int power_below = 0.0, power_above = 0.0;
		if (speed <= SPEEDS[0] + 1e-9) {
			speed_below = SPEEDS[0];
			speed_above = SPEEDS[1];
			power_below = POWERS[0];
			power_above = POWERS[1];
		} else {
			for (std::size_t i = 0; i < G_N_ELEMENTS(SPEEDS) - 1 && SPEEDS[i] < speed; ++i) {
				speed_below = SPEEDS[i];
				speed_above = SPEEDS[i + 1];
				power_below = POWERS[i];
				power_above = POWERS[i + 1];
			}
		}
		double diff_speed = speed_above - speed_below;
		double diff_power = power_above - power_below;
		double slope = diff_power / diff_speed;
		double power = (speed - speed_below) * slope + power_below;
		return static_cast<unsigned int>(clamp(power, 0.0, 4064.0));
	}
}

const std::pair<Point, double> &Player::destination() const {
	return destination_;
}

Point Player::target_velocity() const {
	return target_velocity_;
}

bool Player::has_ball() const {
	return bot->ball_in_beam;
}

bool Player::chicker_ready() const {
	return bot->alive && bot->capacitor_charged;
}

void Player::kick_impl(double speed) {
	if (bot->alive) {
		if (bot->capacitor_charged) {
			bot->kick(calc_kick(speed), 0, 0);
		} else {
			LOG_ERROR(Glib::ustring::compose("Bot %1 chick when not ready", pattern()));
		}
	}
}

void Player::autokick_impl(double speed) {
	if (bot->alive) {
		bot->autokick(calc_kick(speed), 0, 0);
		autokick_invoked = true;
	}
}

Player::Ptr Player::create(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot) {
	Ptr p(new Player(backend, pattern, bot));
	return p;
}

Player::Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot) : AI::BE::XBee::Robot(backend, pattern), bot(bot), controlled(false), dribble_distance_(0.0), battery_warning_hysteresis(-BATTERY_HYSTERESIS_MAGNITUDE), battery_warning_message(Glib::ustring::compose("Bot %1 low battery", pattern), Annunciator::Message::TriggerMode::LEVEL), autokick_invoked(false) {
	timespec now;
	timespec_now(now);
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
}

void Player::drive(const int(&w)[4]) {
	for (unsigned int i = 0; i < 4; ++i) {
		wheel_speeds_[i] = w[i];
	}
	controlled = true;
}

void Player::tick(bool halt) {
	// Check for emergency conditions.
	if (!bot->alive) {
		halt = true;
	}

	// Check for low battery condition.
	if (bot->alive && bot->has_feedback) {
		// Apply some hysteresis.
		if (bot->battery_voltage < BATTERY_CRITICAL_THRESHOLD) {
			if (battery_warning_hysteresis == BATTERY_HYSTERESIS_MAGNITUDE) {
				battery_warning_message.active(true);
			} else {
				++battery_warning_hysteresis;
			}
		} else {
			if (battery_warning_hysteresis == -BATTERY_HYSTERESIS_MAGNITUDE) {
				battery_warning_message.active(false);
			} else {
				--battery_warning_hysteresis;
			}
		}
	} else {
		battery_warning_message.active(false);
	}

	// Inhibit auto-kick if halted or if the AI didn't renew its interest.
	if (halt || !autokick_invoked) {
		bot->autokick(0, 0, 0);
	}
	autokick_invoked = false;

	// Drivetrain control path.
	if (!halt && moved && controlled) {
		bot->drive(wheel_speeds_);
	} else {
		bot->drive_scram();
	}
	controlled = false;

	// Dribbler should always run except in halt.
	bot->dribble(!halt);

	// Chicker should always charge except in halt.
	bot->enable_chicker(!halt);

	// Calculations.
	if (bot->ball_on_dribbler) {
		dribble_distance_ += (position() - last_dribble_position).len();
	} else {
		dribble_distance_ = 0.0;
	}
	last_dribble_position = position();
}

Visualizable::Colour Player::visualizer_colour() const {
	return Visualizable::Colour(0.0, 1.0, 0.0);
}

Glib::ustring Player::visualizer_label() const {
	return Glib::ustring::format(pattern());
}

