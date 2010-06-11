#include "ai/tactic/pass.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/move.h"
#include "ai/util.h"

#include <iostream>

pass::pass(player::ptr player, world::ptr world, player::ptr receiver) : tactic(player), the_world(world), the_receiver(receiver) {
}

void pass::tick() {
	bool should_pass = ai_util::can_pass(the_world, the_receiver) && the_receiver->est_velocity().len() < ai_util::VEL_CLOSE;

	if (should_pass) {
		// std::cout << " pass: let's shoot " << std::endl;
		kick::ptr tactic(new kick(the_player, the_world));
		tactic->set_target(the_receiver->position());
		tactic->tick();
	} else {
		// std::cout << " pass: move to receiver " << std::endl;
		move::ptr tactic(new move(the_player, the_world));
		// ALSO FACE TOWARDS RECEIVER
		tactic->set_position(the_receiver->position());
		if (ai_util::has_ball(the_player)) {
			const point diff = the_receiver->position() - the_player->position();
			tactic->set_orientation(diff.orientation());
		}
		tactic->tick();
	}
}

