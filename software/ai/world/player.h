#ifndef AI_WORLD_PLAYER_H
#define AI_WORLD_PLAYER_H

#include "ai/world/robot.h"
#include "robot_controller/robot_controller.h"
#include "xbee/client/drive.h"
#include <ctime>

class ai;
class world;

/**
 * A player is a robot that can be driven.
 */
class player : public robot {
	public:
		/**
		 * A pointer to a player.
		 */
		typedef Glib::RefPtr<player> ptr;

		/**
		 * Instructs the player to move.
		 * \param dest the destination point to move to
		 * \param ori the target origin to rotate to
		 */
		void move(const point &dest, double ori);

		/**
		 * Sets the speed of the dribbler motor. If this is not invoked by the
		 * AI in a particular time tick, the dribbler will turn off.
		 * \param speed the speed to run at, from 0 to 1
		 */
		void dribble(double speed);

		/**
		 * Fires the kicker.
		 * \param power the power to kick at, from 0 to 1
		 */
		void kick(double power);

		/**
		 * Fires the chipper.
		 * \param power the power to chip at, from 0 to 1
		 */
		void chip(double power);

		/**
		 * \return true if the player senses the ball.
		 * WARNING!!! This can be a false positive,
		 * especially if the dribbler is spinning up or down.
		 */
		bool sense_ball() const {
			return sense_ball_;
		}

		/**
		 * \return The number of seconds for which the player has sensed the ball.
		 */
		double sense_ball_time() const;
	
		/**
		 * \return The number of seconds elapsed since the player has sensed the ball.
		 * May stop the AI from panicking if the player losses the ball temporarily.
		 */
		double last_sense_ball_time() const;
	
		/**
		 * \return The speed the dribbler would be spinning at given the
		 * current power level if it were spinning unloaded and if it had been
		 * given sufficient time to stabilize, in RPM
		 */
		unsigned int theory_dribbler_speed() const {
			return theory_dribble_rpm;
		}

		/**
		 * \return The speed the dribbler is running at, in RPM
		 */
		unsigned int dribbler_speed() const {
			return bot->dribbler_speed();
		}

		/**
		 * \return the distance this player has travelled while dribbling the
		 * ball, in metres (or \c 0.0 if the player is not dribbling now).
		 */
		double dribble_distance() const {
			return dribble_distance_;
		}

		/**
		 * \return Whether the dribbler is safe to use, i.e. won't stall the
		 * motor for too long.
		 */
		bool dribbler_safe() const;

	private:
		xbee_drive_bot::ptr bot;
		point destination_;
		double target_orientation;
		robot_controller2::ptr controller;
		bool moved;
		int new_dribble_power;
		int old_dribble_power;
		bool sense_ball_;
		bool dribble_stall;
		unsigned int theory_dribble_rpm;
		timespec sense_ball_start, sense_ball_end, stall_start, recover_time_start;
		double dribble_distance_;
		point last_dribble_position;

		/**
		 * Constructs a new player object.
		 * \param bot the XBee robot being driven
		 * \return the new object
		 */
		static ptr create(const Glib::ustring &name, bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot);

		/**
		 * Constructs a new player object.
		 * \param bot the XBee robot being driven
		 */
		player(const Glib::ustring &name, bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot);

		/**
		 * Drives one tick of time through the robot_controller and to the XBee.
		 * \param scram whether or not to scram the robot
		 */
		void tick(bool scram);

		visualizable::colour visualizer_colour() const {
			return visualizable::colour(0.0, 1.0, 0.0);
		}

		bool has_destination() const {
			return true;
		}

		point destination() const {
			return destination_;
		}
						
		/**
		 * Process that makes sure that the dribble motor is not stalled for too
		 * long.
		 */
		void dribbler_safety();

		void on_feedback();

		friend class ai;
		friend class world;
};

#endif

