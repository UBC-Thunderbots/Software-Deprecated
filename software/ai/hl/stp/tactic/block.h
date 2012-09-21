#ifndef AI_HL_STP_TACTIC_BLOCK_H
#define AI_HL_STP_TACTIC_BLOCK_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/enemy.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Blocks against an enemy from view of our goal.
				 */
				Tactic::Ptr block_goal(World world, Enemy::Ptr enemy);

				/**
				 * Blocks against an enemy from the ball / passing.
				 */
				Tactic::Ptr block_ball(World world, Enemy::Ptr enemy);
			}
		}
	}
}

#endif

