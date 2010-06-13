#ifndef AI_ROLE_H
#define AI_ROLE_H

#include "ai/world/world.h"
#include "util/byref.h"
#include "ai/flags.h"
#include <vector>
#include <glibmm.h>

/**
 * A role manages the operation of a small group of players.
 */
class role2 : public byref, public sigc::trackable {
	public:
		/**
		 * A pointer to a role.
		 */
		typedef Glib::RefPtr<role2> ptr;

		/**
		 * Runs the role for one time tick. It is expected that the role will
		 * examine the robots for which it is responsible, determine if they
		 * need to be given new tactics, and then call tactic::tick() for all
		 * the tactics under this role.
		 *
		 * It is possible that the set of robots controlled by the tactic may
		 * change between one tick and the next. The role must be ready to deal
		 * with this situation, and must be sure to destroy any tactics
		 * controlling robots that have gone away. This situation can be
		 * detected by implementing robots_changed(), which will be called
		 * whenever the set of robots changes.
		 *
		 * \param[in] overlay the visualizer's overlay on which to draw
		 * graphical information.
		 */
		virtual void tick(Cairo::RefPtr<Cairo::Context> overlay) = 0;

		/**
		 * Called each time the set of robots for which the role is responsible
		 * has changed. It is expected that the role will examine the robots and
		 * determine if any changes need to be made.
		 */
		virtual void robots_changed() = 0;
		
		/**
		 * Sets the robots controlled by this role.
		 *
		 * \param[in] robots the robots the role should control.
		 */
		void set_robots(const std::vector<player::ptr> &robots) {
			the_robots = robots;
			robots_changed();
		}
		
		/**
		 * Removes all robots from this role.
		 */
		void clear_robots() {
			the_robots.clear();
			robots_changed();
		}

	protected:
		/**
		 * The robots that this role controls.
		 */
		std::vector<player::ptr> the_robots;
};

/**
 * A compatibility shim for roles that do not present a visual overlay.
 */
class role : public role2 {
	public:
		/**
		 * A pointer to a role.
		 */
		typedef Glib::RefPtr<role> ptr;

		/**
		 * Runs the role for one time tick. It is expected that the role will
		 * examine the robots for which it is responsible, determine if they
		 * need to be given new tactics, and then call tactic::tick() for all
		 * the tactics under this role.
		 *
		 * It is possible that the set of robots controlled by the tactic may
		 * change between one tick and the next. The role must be ready to deal
		 * with this situation, and must be sure to destroy any tactics
		 * controlling robots that have gone away. This situation can be
		 * detected by implementing robots_changed(), which will be called
		 * whenever the set of robots changes.
		 */
		virtual void tick() = 0;

	private:
		void tick(Cairo::RefPtr<Cairo::Context>) {
			tick();
		}
};

#endif

