#ifndef AI_NAVIGATOR_WORLD_H
#define AI_NAVIGATOR_WORLD_H

#include "ai/common/world.h"
#include "ai/flags.h"
#include "util/byref.h"
#include <utility>
#include <vector>

namespace AI {
	namespace Nav {
		namespace W {
			/**
			 * The field, as seen by a Navigator.
			 */
			class Field : public AI::Common::Field {
			};

			/**
			 * The ball, as seen by a Navigator.
			 */
			class Ball : public AI::Common::Ball {
			};

			/**
			 * A robot, as seen by a Navigator.
			 */
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * \brief Returns the avoidance distance for this robot.
					 *
					 * \return the avoidance distance.
					 */
					virtual AI::Flags::AvoidDistance avoid_distance() const = 0;
			};

			/**
			 * A player, as seen by a Navigator.
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * A pointer to a const Player.
					 */
					typedef RefPtr<const Player> CPtr;

					/**
					 * The type of a single point in a path.
					 */
					typedef std::pair<std::pair<Point, double>, timespec> PathPoint;

					/**
					 * The type of a complete path.
					 */
					typedef std::vector<PathPoint> Path;

					/**
					 * The maximum linear velocity of the robot, in metres per second.
					 */
					static const double MAX_LINEAR_VELOCITY;

					/**
					 * The maximum linear acceleration of the robot, in metres per second squared.
					 */
					static const double MAX_LINEAR_ACCELERATION;

					/**
					 * The maximum angular velocity of the robot, in radians per second.
					 */
					static const double MAX_ANGULAR_VELOCITY;

					/**
					 * The maximum angular acceleration of the robot, in radians per second squared.
					 */
					static const double MAX_ANGULAR_ACCELERATION;

					/**
					 * Returns the destination position and orientation requested by the Strategy.
					 *
					 * \return the destination position and orientation.
					 */
					virtual const std::pair<Point, double> &destination() const = 0;

					/**
					 * Returns the target velocity requested by the Strategy.
					 *
					 * \return the target velocity.
					 */
					virtual Point target_velocity() const = 0;

					/**
					 * Returns the movement flags requested by the Strategy.
					 *
					 * \return the flags.
					 */
					virtual unsigned int flags() const = 0;

					/**
					 * Returns the movement type requested by the Strategy.
					 *
					 * \return the type.
					 */
					virtual AI::Flags::MoveType type() const = 0;

					/**
					 * Returns the movement priority requested by the Strategy.
					 *
					 * \return the priority.
					 */
					virtual AI::Flags::MovePrio prio() const = 0;

					/**
					 * Sets the path this player should follow.
					 *
					 * \param[in] p the path (an empty path causes the robot to halt).
					 */
					virtual void path(const Path &p) = 0;
			};

			/**
			 * The friendly team.
			 */
			class FriendlyTeam : public AI::Common::Team {
				public:
					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					Player::Ptr get(std::size_t i) {
						return get_navigator_player(i);
					}

					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					Player::CPtr get(std::size_t i) const {
						return get_navigator_player(i);
					}

				protected:
					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::Ptr get_navigator_player(std::size_t i) = 0;

					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::CPtr get_navigator_player(std::size_t i) const = 0;
			};

			/**
			 * The enemy team.
			 */
			class EnemyTeam : public AI::Common::Team {
				public:
					/**
					 * Returns a robot from the team.
					 *
					 * \param[in] i the index of the robot.
					 *
					 * \return the robot.
					 */
					Robot::Ptr get(std::size_t i) const {
						return get_navigator_robot(i);
					}

				protected:
					/**
					 * Returns a robot from the team.
					 *
					 * \param[in] i the index of the robot.
					 *
					 * \return the robot.
					 */
					virtual Robot::Ptr get_navigator_robot(std::size_t i) const = 0;
			};

			/**
			 * The world, as seen by a Navigator.
			 */
			class World {
				public:
					/**
					 * Returns the field.
					 *
					 * \return the field.
					 */
					virtual const Field &field() const = 0;

					/**
					 * Returns the ball.
					 *
					 * \return the ball.
					 */
					virtual const Ball &ball() const = 0;

					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual FriendlyTeam &friendly_team() = 0;

					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual const FriendlyTeam &friendly_team() const = 0;

					/**
					 * Returns the enemy team.
					 *
					 * \return the enemy team.
					 */
					virtual const EnemyTeam &enemy_team() const = 0;

					/**
					 * Returns the current monotonic time.
					 * Monotonic time is a way of representing "game time", which always moves forward.
					 * Monotonic time is consistent within the game world, and may or may not be linked to real time.
					 * A navigator should \em always use this function to retrieve monotonic time, not one of the functions in util/time.h!
					 * The AI will not generally have any use for real time.
					 *
					 * \return the current monotonic time.
					 */
					virtual timespec monotonic_time() const = 0;
			};
		}
	}
}

#endif

