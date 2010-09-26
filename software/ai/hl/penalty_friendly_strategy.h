#ifndef AI_HL_PENALTY_H
#define AI_HL_PENALTY_H

#include "ai/hl/strategy.h"

#include <vector>

namespace AI {
	namespace HL {
		/**
		 * Gets the robots to go to their penalty_friendly positions.
		 */
		class PenaltyFriendly {
			public:
				/**
		 		 * A pointer to a penalty_friendly role.
				 */
				typedef RefPtr<PenaltyFriendly> ptr;

				/**
			 	 * Constructs a new penalty_friendly role.
			 	 */
				PenaltyFriendly(AI::HL::W::World &world);

			protected:
				AI::HL::W::World &world;

				/**
		 		 * The distance between the penalty mark and the mid point of the two goal posts as described in the rules.
		 		 */
				const static double PENALTY_MARK_LENGTH = 0.45;

				/**
		 		 * The distance between the baseline and the line behind which other robots may stand.
				 */
				const static double RESTRICTED_ZONE_LENGTH = 0.85;

				/**
				 * Maximum number of robots that can be assigned to this role.
				 */
				const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

			        /**
				 * The positions that the robots should move to for this role.
				 */
				Point ready_positions [NUMBER_OF_READY_POSITIONS];
					
		};
	}
}

#endif

