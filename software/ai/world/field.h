#ifndef AI_WORLD_FIELD_H
#define AI_WORLD_FIELD_H

#include "proto/messages_robocup_ssl_geometry.pb.h"
#include "uicomponents/visualizer.h"
#include "util/noncopyable.h"

class world;

/**
 * Provides the AI with a way to get the dimensions of the field. The dimensions
 * of the field are provided by SSL-Vision or the simulator.
 */
class field : public visualizable::field {
	public:
		/**
		 * \return The length of the field, from goal to goal, in metres
		 */
		double length() const {
			return length_;
		}

		/**
		 * \return The length of the field, including the boundary and referee
		 * area, in metres
		 */
		double total_length() const {
			return total_length_;
		}

		/**
		 * \return The width of the field, from sideline to sideline, in metres
		 */
		double width() const {
			return width_;
		}

		/**
		 * \return The width of the field, including the boundary and referee
		 * area, in metres
		 */
		double total_width() const {
			return total_width_;
		}

		/**
		 * \return The width of the goal, symmetric above and below the
		 * centreline, from goalpost to goalpost, in metres
		 */
		double goal_width() const {
			return goal_width_;
		}

		/**
		 * \return The radius of the centre circle, in metres
		 */
		double centre_circle_radius() const {
			return centre_circle_radius_;
		}

		/**
		 * \return The radius of the arcs at the top and bottom of the defense
		 * area, in metres
		 */
		double defense_area_radius() const {
			return defense_area_radius_;
		}

		/**
		 * \return The width of the straight part of the defense area between
		 * the two arcs, in metres
		 */
		double defense_area_stretch() const {
			return defense_area_stretch_;
		}

	private:
		double length_;
		double total_length_;
		double width_;
		double total_width_;
		double goal_width_;
		double centre_circle_radius_;
		double defense_area_radius_;
		double defense_area_stretch_;

		/**
		 * Constructs a new field object.
		 */
		field();

		/**
		 * Updates the field object with new geometry data from SSL-Vision or
		 * the simulator.
		 *
		 * \param packet packet the new data
		 */
		void update(const SSL_GeometryFieldSize &packet);

		friend class world;
};

#endif

