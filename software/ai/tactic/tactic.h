#ifndef AI_TACTIC_H
#define AI_TACTIC_H

#include "util/byref.h"
#include "ai/world/player.h"
#include <glibmm.h>

/**
 * A tactic controls the operation of a single player doing some activity.
 */
class tactic : public byref {
	public:
		/**
		 * A pointer to a tactic.
		 */
		typedef Glib::RefPtr<tactic> ptr;

		/**
		 * Runs the tactic for one time tick. It is expected that the tactic
		 * will examine the robot for which it is responsible, determine how it
		 * wishes that robot to move, and then do one of the following:
		 *
		 * If this tactic is layered on top of another tactic, call
		 * tactic::tick() on that lower-level tactic, OR
		 *
		 * If this tactic is the bottom-level tactic and is layered on top of a
		 * navigator, call navigator::set_point() and then navigator::tick(), OR
		 *
		 * If this tactic is the bottom-level tactic and does not use a
		 * navigator, call player::move().
		 */
		virtual void tick() = 0;

		/**
		 * Set flags that restrict the movement of the robots.
		 * Flags are permanent once set.
		 */
		void set_flags(const unsigned int& f) {
			flags |= f;
		}

	protected:
		/**
		 * Constructor, flags set to 0 by default.
		 */
		tactic() : flags(0) {
		}

		explicit tactic(player::ptr player) : flags(0), the_player(player) {
		}

		/**
		 * Constructor, with flags argument.
		 */
		explicit tactic(const unsigned int& f) : flags(f) {
		}

		explicit tactic(player::ptr player, const unsigned int& f) : flags(f), the_player(player) {
		}

		unsigned int flags;
		player::ptr the_player;
};

#endif

