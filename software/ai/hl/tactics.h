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
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags, const Point target);

			/**
			 * I can't think of a good name for this function.
			 *
			 * Anyways, tries to move the ball as far away from friendly goal.
			 * If ball is not in possesion, chase after the ball.
			 * Otherwise, shoot the ball in the furthest possible direction.
			 * Useful for defenders.
			 *
			 * \param[in] flags movement flags for the robot
			 * most likely you want to disable avoid_friendly_defense.
			 */
			void repel(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags);
		}
		/**
		 *
		 * Helper Class to let a player (normally the goalie) to patrol between two points on the field	
		 *
		 * Still Under Construction	
		 */
		class Patrol{
			public:
				typedef RefPtr<Patrol> ptr;
	
				Patrol(W::Player::Ptr player, W::World &w);

				/**
				 *
				 * Patrol between two points on the field
				 *
				 * \param[in] flags movement flags for the robot
				 * most likely you want to disable avoid_friendly_defense.
				 */
				Patrol(W::Player::Ptr player, W::World &w, const unsigned int& flags, const Point& t1, const Point& t2);

				/**
				 * Set the two target points for the patrol.
				 */
				void set_targets(const Point& t1, const Point& t2) {
					target1 = t1;
					target2 = t2;
					target_initialized = true;
				}


				void tick();	
		
			protected:		

				Point target1, target2;
				bool target_initialized;

				
		};
	}
}

#endif

