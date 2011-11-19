#ifndef AI_HL_STP_TACTIC_MOVE_STOP_H
#define AI_HL_STP_TACTIC_MOVE_STOP_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to correct stopping location for the specified player index.
				 *
				 * \param[in] player_index goes from 0 to 3 and is used for calculating a robot's relative
				 * position around the ball.
				 */
				Tactic::Ptr move_stop(const World &world, int player_index);

				class StopLocations : public Cacheable<std::vector<Point>, CacheableNonKeyArgs<const AI::HL::W::World &>, CacheableKeyArgs<> > {
					protected:
						std::vector<Point> compute(const World &w);
				};

				extern StopLocations stop_locations;
			}
		}
	}
}

#endif

