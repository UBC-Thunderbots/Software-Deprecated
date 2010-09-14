#ifndef AI_HL_WORLD_H
#define AI_HL_WORLD_H

#include "ai/common/world.h"
#include "util/property.h"
#include <sigc++/sigc++.h>

namespace AI {
	namespace HL {
		namespace W {
			namespace PlayType = AI::Common::PlayType;

			/**
			 * The field, as seen by a Strategy.
			 */
			class Field : public AI::Common::Field {
			};
			
			/**
			 * The ball, as seen by a Strategy.
			 */
			class Ball : public AI::Common::Ball {
			};

			/**
			 * A robot, as seen by a Strategy.
			 */
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * A pointer to a Robot.
					 */
					typedef RefPtr<Robot> Ptr;
			};

			/**
			 * A player, as seen by a Strategy.
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * Sets the current destination and movement type for this player.
					 *
					 * \param[in] dest the destination position to move to.
					 *
					 * \param[in] ori the target orientation to assume.
					 *
					 * \param[in] flags the flags governing the movement.
					 */
					virtual void move(Point dest, double ori, unsigned int flags) = 0;
			};

			/**
			 * The friendly team.
			 */
			class FriendlyTeam : public AI::Common::Team {
				public:
					/**
					 * Returns a player from the team.
					 *
					 * \param i the index of the player.
					 *
					 * \return the player.
					 */
					Player::Ptr get(std::size_t i) {
						return get_hl_player(i);
					}

				protected:
					/**
					 * Returns a player from the team.
					 *
					 * \param i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::Ptr get_hl_player(std::size_t i) = 0;
			};

			/**
			 * The enemy team.
			 */
			class EnemyTeam : public AI::Common::Team {
				public:
					/**
					 * Returns a robot from the team.
					 *
					 * \param i the index of the robot.
					 *
					 * \return the robot.
					 */
					Robot::Ptr get(std::size_t i) {
						return get_hl_robot(i);
					}

				protected:
					/**
					 * Returns a robot from the team.
					 *
					 * \param i the index of the robot.
					 *
					 * \return the robot.
					 */
					virtual Robot::Ptr get_hl_robot(std::size_t i) = 0;
			};

			/**
			 * The world, as seen by a Strategy.
			 */
			class World {
				public:
					/**
					 * Returns the field.
					 *
					 * \return the field.
					 */
					virtual Field &field() = 0;

					/**
					 * Returns the ball.
					 *
					 * \return the ball.
					 */
					virtual Ball &ball() = 0;

					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual FriendlyTeam &friendly_team() = 0;

					/**
					 * Returns the enemy team.
					 *
					 * \return the enemy team.
					 */
					virtual EnemyTeam &enemy_team() = 0;

					/**
					 * Returns the current play type.
					 *
					 * \return the current play type.
					 */
					virtual const Property<PlayType::PlayType> &playtype() const = 0;

				protected:
					/**
					 * Constructs a new World.
					 */
					World();

					/**
					 * Destroys a World.
					 */
					~World();
			};
		}
	}
}

#endif

