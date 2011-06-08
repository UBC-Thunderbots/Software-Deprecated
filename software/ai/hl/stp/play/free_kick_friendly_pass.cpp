#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/chase.h"

namespace Predicates = AI::HL::STP::Predicates;

BEGIN_PLAY(FreeKickFriendlyPass)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 3) && !Predicates::baller_can_shoot(world) && Predicates::baller_can_pass(world))
APPLICABLE(true)
DONE(Predicates::none_ball(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// passer
roles[0].push_back(chase(world));
roles[0].push_back(passer_shoot(world));

// ROLE 2
// passee
roles[1].push_back(passee_move(world));

// ROLE 3
// defend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));
END_ASSIGN()
END_PLAY()

