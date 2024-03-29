#include <ai/hl/stp/tactic/defend.h>
#include <ai/backend/primitives/all.h>
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/flags.h"

BEGIN_DEC(PenaltyFriendly)
INVARIANT(
    playtype(world, PlayType::PREPARE_PENALTY_FRIENDLY) ||
    playtype(world, PlayType::EXECUTE_PENALTY_FRIENDLY))
APPLICABLE(true)
END_DEC(PenaltyFriendly)

BEGIN_DEF(PenaltyFriendly)
DONE(goal(world))
FAIL(false)
EXECUTE()

tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::move(
    world, Point(world.field().penalty_enemy().x - 2 * Robot::MAX_RADIUS, 0));

Point penalty_position1(0, 1.5);
Point penalty_position2(0, -1.5);
Point penalty_position3(0, -0.75);
Point penalty_position4(0, 0.75);

tactics[2] = Tactic::move(world, penalty_position1);
tactics[3] = Tactic::move(world, penalty_position2);
tactics[4] = Tactic::move(world, penalty_position3);
tactics[5] = Tactic::move(world, penalty_position4);

while (!playtype(world, PlayType::EXECUTE_PENALTY_FRIENDLY))
    yield(caller);

tactics[1] = Tactic::penalty_shoot(world);

//Player player = world.friendly_team()[1];
//AI::BE::Primitives::Ptr prim(
//    new AI::BE::Primitives::Shoot(player, world.ball().position(), 1.0, false));
//
//(static_cast<AI::Common::Player>(player)).impl->push_prim(prim);

while (1)
    yield(caller);

END_DEF(PenaltyFriendly)
