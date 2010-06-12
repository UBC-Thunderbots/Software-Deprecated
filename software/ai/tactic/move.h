#ifndef AI_TACTIC_MOVE_H
#define AI_TACTIC_MOVE_H

#include "ai/navigator/robot_navigator.h"
#include "ai/tactic/tactic.h"

/**
 * A wrapper around robot navigator.
 * I.e. this class exist only so that roles do not call navigator directly.
 * Therefore, me thinks that other tactic should not have instance of this tactic.
 */
class move : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<move> ptr;

		/**
		 * Standard constructor.
		 */
		move(player::ptr player, world::ptr world);

 		/**
-		 * Overloaded constructor with flags option.
 		 */
 		move(player::ptr player, world::ptr world, const unsigned int& flags);

		/**
		 * Most usage of move tactic only sets position and should thus justify existence of this overloaded constructor.
		 * \param position Moves the robot to this position.
		 */
		// move(player::ptr player, world::ptr world, const unsigned int& flags, const point& position);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this move tactic
		 */
		void set_position(const point& p) {
			target_position = p;
			position_initialized = true;
		}

		/**
		 * Gets whether the position is set for this move tactic.
		 */
		bool is_position_set() {
			return position_initialized;
		}

		/**
		 * Sets the target orientation for this move tactic
		 */
		void set_orientation(const double& d) {
			target_orientation = d;
			orientation_initialized = true;
		}

		/**
		 * Makes the robot face the ball.
		 */
		void unset_orientation() {
			orientation_initialized = false;
		}

	protected:		
		robot_navigator navi;

		point target_position;
		double target_orientation;

		bool position_initialized;
		bool orientation_initialized;
};

#endif

