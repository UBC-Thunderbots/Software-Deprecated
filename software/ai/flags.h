#ifndef AI_FLAGS_H
#define AI_FLAGS_H

#include "ai/common/playtype.h"

namespace AI {
	namespace Flags {
		/**
		 * Flags indicating how robots comply with game rules.
		 * Flags are set by the Strategy and examined by the Navigator to determine which potential paths are legal.
		 */
		enum {
			/**
			 * Do not exit the play area of the field.
			 */
			FLAG_CLIP_PLAY_AREA = 0x0001,

			/**
			 * Avoid the ball by 50cm as required by ball-out-of-play rules.
			 */
			FLAG_AVOID_BALL_STOP = 0x0002,

			/**
			 * Avoid the ball very slightly, e.g. to orbit before touching the ball at a kickoff or free kick.
			 */
			FLAG_AVOID_BALL_TINY = 0x0004,

			/**
			 * Do not enter the friendly defence area.
			 */
			FLAG_AVOID_FRIENDLY_DEFENSE = 0x0008,

			/**
			 * Stay at least 20cm outside the enemy defense area as required by ball-entering-play rules.
			 */
			FLAG_AVOID_ENEMY_DEFENSE = 0x0010,

			/**
			 * Do not enter the enemy's field half.
			 */
			FLAG_STAY_OWN_HALF = 0x0020,

			/**
			 * Stay more than 40cm outside the penalty mark line as required for non-kickers in penalty kick rules.
			 */
			FLAG_PENALTY_KICK_FRIENDLY = 0x0040,

			/**
			 * Stay more than 40cm outside the penalty mark line as required for non-goalies in penalty kick rules.
			 */
			FLAG_PENALTY_KICK_ENEMY = 0x0080,

			/**
			 * When we are kicking, need to stay 20 cm away from enemy defense area
			 *
			 * Rules:
			 * "If, at the time the ball enters play,
			 * a member of the kicker's team occupies the area closer than
			 * 200 mm to the opponent's defense area
			 * an indirect free kick is awarded to the opposing team,
			 * the kick to be taken from the location of
			 *	the ball when the infringement occurred"
			 *
			 */
			FLAG_FRIENDLY_KICK = 0x0100,
		};

		/**
		 * The union of all existent flags.
		 */
		extern const unsigned int FLAGS_VALID;

		/**
		 * Returns the correct flags for a common player,
		 * i.e. a player that is \em not a goalie or the taker of a free kick, kickoff, or penalty kick.
		 * The caller should modify the returned value for the goalie or kicker.
		 *
		 * \param[in] pt the current play type.
		 *
		 * \return the appropriate flags.
		 */
		unsigned int calc_flags(AI::Common::PlayType pt);

		/**
		 * Movement types indicating styles of movement for robots to take.
		 */
		enum class MoveType {
			/**
			 * Move to the target location and orientation as quickly as possible.
			 */
			NORMAL,

			/**
			 * Move to the target location and orientation slowly so as not to drop the ball.
			 */
			DRIBBLE,

			/**
			 * Move to intercept the ball as soon as possible, ignoring target location but intercepting with the requested orientation.
			 * Ignores the target velocity.
			 */
			CATCH,

			/**
			 * Move to the target location and orientation at the same time the ball will get there, but don't worry about stopping at the target after.
			 * Ignores the target velocity.
			 */
			RAM_BALL,

			/**
			 * Stop moving (not intended for use by high-levels, only for things like emergency stop).
			 * Ignores the target velocity.
			 */
			HALT,

			/**
			 * Pivot around the ball.
			 * Orients towards the target, but ignores target velocity.
			 */
			PIVOT,
		};

		/**
		 * Movement priorities indicating which robots should give way so other robots can drive in a straight line.
		 */
		enum class MovePrio {
			/**
			 * An important movement which should not be diverted, such as handling the ball.
			 */
			HIGH,

			/**
			 * An ordinary everyday movement, such as getting into the open to receive a pass.
			 */
			MEDIUM,

			/**
			 * An unimportant movement, such as moving to the other side of the field for a long-term strategic purpose.
			 */
			LOW,
		};
	}
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority precedes another priority if it is more important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is more important than \p b.
 */
inline bool operator<(AI::Flags::MovePrio a, AI::Flags::MovePrio b) {
	return static_cast<unsigned int>(a) < static_cast<unsigned int>(b);
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority succedes another priority if it is less important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is less important than \p b.
 */
inline bool operator>(AI::Flags::MovePrio a, AI::Flags::MovePrio b) {
	return static_cast<unsigned int>(a) > static_cast<unsigned int>(b);
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority precedes another priority if it is more important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is more important than or equal to \p b.
 */
inline bool operator<=(AI::Flags::MovePrio a, AI::Flags::MovePrio b) {
	return a < b || a == b;
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority succedes another priority if it is less important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is less important than or equal to \p b.
 */
inline bool operator>=(AI::Flags::MovePrio a, AI::Flags::MovePrio b) {
	return a > b || a == b;
}

#endif

