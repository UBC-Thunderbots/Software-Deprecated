#ifndef AI_BACKEND_ROBOT_H
#define AI_BACKEND_ROBOT_H

#include "ai/flags.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "uicomponents/visualizer.h"
#include "util/box_ptr.h"
#include "util/object_store.h"
#include "util/predictor.h"
#include <glibmm/ustring.h>

namespace AI {
	namespace BE {
		/**
		 * \brief A robot, as exposed by the backend
		 */
		class Robot : public Visualizable::Robot {
			public:
				/**
				 * \brief A pointer to a Robot
				 */
				typedef BoxPtr<Robot> Ptr;

				/**
				 * \brief A pointer to a const Robot
				 */
				typedef BoxPtr<const Robot> CPtr;

				explicit Robot(unsigned int pattern);

				/**
				 * \brief Updates the position of the robot using new field data
				 *
				 * \param[in] pos the position of the robot
				 *
				 * \param[in] ori the orientation of the robot
				 *
				 * \param[in] ts the time at which the robot was in the given position
				 */
				void add_field_data(Point pos, Angle ori, timespec ts);

				ObjectStore &object_store() const;
				unsigned int pattern() const;
				Point position(double delta) const;
				Point position_stdev(double delta) const;
				Angle orientation(double delta) const;
				Angle orientation_stdev(double delta) const;
				Point velocity(double delta) const;
				Point velocity_stdev(double delta) const;
				Angle avelocity(double delta) const;
				Angle avelocity_stdev(double delta) const;
				bool has_destination() const;
				std::pair<Point, Angle> destination() const;
				bool has_path() const;
				const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &path() const;
				void avoid_distance(AI::Flags::AvoidDistance dist) const;
				AI::Flags::AvoidDistance avoid_distance() const;
				void pre_tick();
				void lock_time(timespec now);

				Visualizable::Colour visualizer_colour() const;
				Glib::ustring visualizer_label() const;
				bool highlight() const;
				Visualizable::Colour highlight_colour() const;
				unsigned int num_bar_graphs() const;
				double bar_graph_value(unsigned int index) const;
				Visualizable::Colour bar_graph_colour(unsigned int index) const;

			protected:
				Predictor3 pred;

			private:
				const unsigned int pattern_;
				mutable ObjectStore object_store_;
				mutable AI::Flags::AvoidDistance avoid_distance_;
		};
	}
}



inline void AI::BE::Robot::avoid_distance(AI::Flags::AvoidDistance dist) const {
	avoid_distance_ = dist;
}

inline AI::Flags::AvoidDistance AI::BE::Robot::avoid_distance() const {
	return avoid_distance_;
}

#endif
