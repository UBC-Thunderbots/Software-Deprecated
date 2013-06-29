#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/repel.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;

/**
 * Condition:
 * - Playtype Play
 *
 * Objective:
 * - just repel the ball to the enemy side
 */
BEGIN_PLAY(JustRepel)
INVARIANT(playtype(world, AI::Common::PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// repel
roles[0].push_back(repel(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// defend extra
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// block
roles[3].push_back(block_ball(world, Enemy::closest_ball(world, 0)));

// ROLE 5 (optional)
// offend
roles[4].push_back(offend(world));

END_ASSIGN()
END_PLAY()

