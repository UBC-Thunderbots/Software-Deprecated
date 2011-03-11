#ifndef AI_HL_STP_EVALUATION_PASS_H
#define AI_HL_STP_EVALUATION_PASS_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "util/cacheable.h"

#include <set>
#include <utility>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
			
				std::pair <Point, Point> calc_pass_positions(const World &world, const std::set<Player::CPtr> &players);
				
				/**
				 * Calculates strategic positions to place offensive players.
				 * - Finds weak points on the enemy goal area,
				 *   where the enemy net is open and the ball is visible.
				 * - Places that are near players are preferred.
				 * - Additionally, if some other player is visible to the goal area,
				 *   will avoid blocking view of it.
				 *
				 * \param[in] players the set of players that influence the output.
				 */
				//Point evaluate_offense(const AI::HL::W::World &world, const std::set<AI::HL::W::Player::Ptr> &players);
			}
		}
	}
}

#endif

