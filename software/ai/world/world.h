#ifndef AI_WORLD_WORLD_H
#define AI_WORLD_WORLD_H

#include "ai/world/ball.h"
#include "ai/world/field.h"
#include "ai/world/player.h"
#include "ai/world/playtype.h"
#include "ai/world/team.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
#include "util/clocksource.h"
#include "util/config.h"
#include "util/fd.h"
#include "xbee/client/drive.h"
#include <vector>
#include <sigc++/sigc++.h>

/**
 * Collects all information the AI needs to examine the state of the world and
 * transmit orders to robots.
 */
class world : public byref {
	public:
		/**
		 * A pointer to a world object.
		 */
		typedef Glib::RefPtr<world> ptr;

		/**
		 * The configuration file.
		 */
		const config &conf;

		/**
		 * Fired when a detection packet is received.
		 */
		sigc::signal<void> signal_detection;

		/**
		 * Fired when a field geometry packet is received.
		 */
		sigc::signal<void> signal_geometry;

		/**
		 * Fired when the play type changes.
		 */
		sigc::signal<void> signal_playtype_changed;

		/**
		 * Fired when the current team switches ends.
		 */
		sigc::signal<void> signal_flipped_ends;

		/**
		 * Fired when the local team's colour with respect to the referee box
		 * changes.
		 */
		sigc::signal<void> signal_flipped_refbox_colour;

		/**
		 * The friendly team.
		 */
		friendly_team friendly;

		/**
		 * The enemy team.
		 */
		enemy_team enemy;

		/**
		 * Creates a new world object.
		 * \param conf the configuration file
		 * \param xbee_bots the robots to drive
		 * \return The new object
		 */
		static ptr create(const config &conf, const std::vector<xbee_drive_bot::ptr> &xbee_bots);

		/**
		 * \return The ball
		 */
		::ball::ptr ball() const {
			return ball_;
		}

		/**
		 * \return The field
		 */
		const class field &field() const {
			return field_;
		}

		/**
		 * \return Which end of the physical field the local team is defending
		 */
		bool east() const {
			return east_;
		}

		/**
		 * Inverts which end of the physical field the local team is defending.
		 */
		void flip_ends();

		/**
		 * \return Which colour the local team considers itself to be with
		 * respect to referee box commands
		 */
		bool refbox_yellow() const {
			return refbox_yellow_;
		}

		/**
		 * Inverts which colour the local team should be considered to be with
		 * respect to referee box commands.
		 */
		void flip_refbox_colour();

		/**
		 * \return The current state of play
		 */
		playtype::playtype playtype() const {
			return playtype_;
		}

		/**
		 * \return A visualizable view of the world
		 */
		const visualizable &visualizer_view() const {
			return vis_view;
		}

	private:
		class visualizer_view : public visualizable {
			public:
				visualizer_view(const world * const w) : the_world(w) {
				}

				const class visualizable::field &field() const {
					return the_world->field();
				}

				visualizable::ball::ptr ball() const {
					return the_world->ball();
				}

				std::size_t size() const {
					return the_world->friendly.size() + the_world->enemy.size();
				}

				visualizable::robot::ptr operator[](unsigned int index) const {
					if (index < the_world->friendly.size()) {
						return the_world->friendly.get_robot(index);
					} else {
						return the_world->enemy.get_robot(index - the_world->friendly.size());
					}
				}

			private:
				const world * const the_world;
		};

		bool east_;
		bool refbox_yellow_;
		const file_descriptor vision_socket;
		const file_descriptor refbox_socket;
		class field field_;
		ball::ptr ball_;
		SSL_DetectionFrame detections[2];
		const std::vector<xbee_drive_bot::ptr> xbee_bots;
		playtype::playtype playtype_;
		class visualizer_view vis_view;

		world(const config &, const std::vector<xbee_drive_bot::ptr> &);
		bool on_vision_readable(Glib::IOCondition);
		bool on_refbox_readable(Glib::IOCondition);
		void playtype(playtype::playtype);
};

#endif

