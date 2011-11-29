#ifndef AI_HL_STP_TACTIC_BALL_H
#define AI_HL_STP_TACTIC_BALL_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/region.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {

				/**
				 * Steal
				 */
				Tactic::Ptr tsteal(const World &world);

				/**
				 * Active Defense
				 */
				Tactic::Ptr tactive_def(const World &world);

				/**
				 * Dribble to Region
				 */
				Tactic::Ptr tdribble_to_region(const World &world, Region _region);

				/**
				 * Spin to Region
				 */
				Tactic::Ptr tspin_to_region(const World &world, Region _region);
			}
		}
	}
}

#endif
