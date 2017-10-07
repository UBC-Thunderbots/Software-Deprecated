#include "../tactic/ball.h"
#include "../tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"

BEGIN_DEC(BlockShotPath)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(defensive(world) && none_ball(world) && ball_x_less_than(world, 0))
END_DEC(BlockShotPath)

BEGIN_DEF(BlockShotPath)
DONE(our_ball(world))
FAIL(their_ball(world))
EXECUTE()
tactics[0] = Tactic::lone_goalie(world);
tactics[1] = Tactic::tactive_def(world);
tactics[2] = Tactic::defend_duo_defender(world);
tactics[3] = Tactic::block_shot_path(world, 0);
tactics[4] = Tactic::block_shot_path(world, 1);
tactics[5] = Tactic::block_shot_path(world, 2);

wait(caller, tactics[1].get());
END_DEF(BlockShotPath)
