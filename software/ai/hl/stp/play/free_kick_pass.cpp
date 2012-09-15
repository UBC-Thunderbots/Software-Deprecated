#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/free_kick_pass.h"
#include "ai/hl/stp/tactic/wait_playtype.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Playtype Free Kick Friendly
 *
 * Objective:
 * - Handle Friendly Free Kick by passing. 
 */
BEGIN_PLAY(FreeKickPass)
INVARIANT(
		(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
		 || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)
		 || Predicates::playtype(world, AI::Common::PlayType::PLAY))
		&& Predicates::our_team_size_at_least(world, 2))
APPLICABLE((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
			|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY))
			&& !Predicates::ball_in_their_corner(world) && !Predicates::ball_in_our_corner(world))
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(free_kick_pass(world,world.field().enemy_goal()));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// offend
roles[2].push_back(offend(world));

// ROLE 4
// offend
roles[3].push_back(offend_secondary(world));

// ROLE 5
// extra defender
roles[4].push_back(defend_duo_extra1(world));

END_ASSIGN()
END_PLAY()

