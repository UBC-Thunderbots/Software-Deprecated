#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - ball under team possesion
 * - have at least 4 players (one goalie, one passer, one passee, one defender)
 *
 * Objective:
 * - shoot the ball to enemy goal while passing the ball between the passer and passee
 */
BEGIN_PLAY(PassOffensive)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && Predicates::their_team_size_at_least(world, 1) && !Predicates::baller_can_shoot(world) && !Predicates::fight_ball(world))
APPLICABLE(Predicates::our_ball(world) && (Predicates::ball_midfield(world) || Predicates::ball_in_their_corner(world)) && Predicates::ball_on_their_side(world))
DONE(Predicates::baller_can_shoot(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 2));

// ROLE 1
// passer / receiver
roles[0].push_back(passer_shoot_dynamic(world));
roles[0].push_back(passee_receive(world));

// ROLE 2
// passee / offender
roles[1].push_back(passee_move_dynamic(world));
roles[1].push_back(offend(world));

// ROLE 3
// defend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offensive support through blocking closest enemy to ball
roles[3].push_back(block_ball(world, Enemy::closest_ball(world, 0)));
END_ASSIGN()
END_PLAY()

