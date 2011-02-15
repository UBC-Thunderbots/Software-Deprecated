#ifndef AI_HL_TACTICS_H
#define AI_HL_TACTICS_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		/**
		 * Tactics are helper methods/classes which allow strategies
		 * to compose commonly used complex movement.
		 *
		 * It also serves to unify definition.
		 *
		 * Hint:
		 * - It is much easier to change the implementation of a tactic here,
		 * instead of changing every usage of the player movement.
		 *
		 */
		namespace Tactics {
			/**
			 * Chases after the ball as fast as possible.
			 */
			void chase(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags);

			/**
			 * If the player posses the ball,
			 * aims at an open angle at the enemy goal,
			 * and shoots the ball.
			 *
			 * If the player does not have the ball, chases after it.
			 *
			 * \param[in] force forces the player to shoot,
			 * even if the goal is completely blocked.
			 */
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags, const bool force = false);

			/**
			 * If the player posses the ball,
			 * aims at the target and shoots the ball.
			 *
			 * If the player does not have the ball, chases after it.
			 *
			 * \param[in] target the location to shoot the ball to.
			 */
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags, const Point target, const double kick_power = 10.0);

			/**
			 * I can't think of a good name for this function.
			 *
			 * Anyways, tries to move the ball as far away from friendly goal.
			 * If ball is not in possesion, chase after the ball.
			 * Otherwise, shoot the ball in the furthest possible direction.
			 * Useful for defenders.
			 *
			 * \param[in] flags movement flags for the robot (most likely you want to disable AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE).
			 */
			void repel(const AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags);

			/**
			 * Convenient function.
			 * Move the players when NOT in any ACTIVE play.
			 * For example, during stop, preparing for kickoff, preparing for free kick.
			 */
			void free_move(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const Point p);

			/**
			 * A single goalie and NO ONE ELSE defending the field.
			 */
			void lone_goalie(const AI::HL::W::World &world, AI::HL::W::Player::Ptr player);

			/**
			 * Blocks against a single enemy from shooting.
			 */
			void block(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags, AI::HL::W::Robot::Ptr robot);

			/**
			 * Passer shoots ball to passee.
			 * Checks if passer has ball and passee can receive ball.
			 */
			void pass(AI::HL::W::World &world, AI::HL::W::Player::Ptr passer, AI::HL::W::Player::Ptr passee, const unsigned int flags);

			/**
			 * Helper Class to let a player (normally the goalie) to patrol between two points on the field
			 */
			class Patrol {
				public:
					typedef RefPtr<Patrol> Ptr;

					Patrol(W::Player::Ptr player, W::World &w);

					/**
					 * Patrol between two points on the field
					 */
					Patrol(AI::HL::W::World &w);

					/**
					 * Sets the player who do the patrol.
					 */
					void set_player(const AI::HL::W::Player::Ptr p) {
						player = p;
					}

					/**
					 * Set the flags for the player.
					 *
					 * \param[in] f movement flags for the robot (most likely you want to disable AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE).
					 */
					void set_flags(const unsigned int f) {
						flags = f;
					}

					/**
					 * Set the two target points for the patrol.
					 */
					void set_targets(const Point &t1, const Point &t2) {
						target1 = t1;
						target2 = t2;
					}

					void tick();

				protected:
					AI::HL::W::World &world;
					AI::HL::W::Player::Ptr player;
					Point target1, target2;
					unsigned int flags;
					bool goto_target1;
			};
		}
	}
}

#endif

