#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/tri_attack.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"

using namespace AI::HL::STP;

namespace AI {
	namespace HL {
		namespace STP {
			extern Player _goalie;
			BoolParam use_gradient_pass(u8"Run pass calculation on seperate thread", u8"AI/HL/STP/PlayExecutor", true);
		
		}
	}
}

void AI::HL::STP::tick_eval(World world) {
	Evaluation::tick_ball(world);
	Evaluation::tick_offense(world);
	Evaluation::tick_defense(world);
	//Evaluation::tick_tri_attack(world);

	//Update version of world used in pass calculation thread
	if( use_gradient_pass && world.friendly_team().size() > 1 && world.enemy_team().size() > 0){
		GradientApproach::PassInfo::Instance().updateWorldSnapshot(world);
	
		if(!GradientApproach::PassInfo::Instance().threadRunning()){
		    GradientApproach::PassInfo::worldSnapshot snapshot = GradientApproach::PassInfo::Instance().getWorldSnapshot();

		    GradientApproach::PassInfo::Instance().setThreadRunning(true);
		    std::thread pass_thread(GradientApproach::superLoop, snapshot);
		    pass_thread.detach();
		    
		    
		    
		}
			
	}
}

void AI::HL::STP::draw_ui(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	draw_shoot(world, ctx);
	draw_offense(world, ctx);
	draw_defense(world, ctx);
	draw_enemy_pass(world, ctx);
	draw_friendly_pass(world, ctx);
	draw_player_status(world, ctx);
	draw_baller(world, ctx);
}

Player AI::HL::STP::get_goalie() {
	return _goalie;
}

