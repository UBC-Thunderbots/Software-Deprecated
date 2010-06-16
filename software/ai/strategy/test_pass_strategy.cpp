#include "ai/strategy/strategy.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"
#include "ai/tactic/kick.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class test_pass_strategy : public strategy {
		public:
			test_pass_strategy(world::ptr world);
			void tick();
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const world::ptr the_world;
	};

	test_pass_strategy::test_pass_strategy(world::ptr world) : the_world(world) {

	}

	void test_pass_strategy::tick() {
		const friendly_team &the_team(the_world->friendly);
		const ball::ptr the_ball(the_world->ball());

		if (the_team.size() != 2) return;

		// player 0 is the receiver
		const player::ptr receiver = the_team.get_player(1);
		move move_tactic(receiver, the_world);
		move_tactic.set_position(point(0.0, 0.0));
		move_tactic.tick();

		bool receiverhasball = ai_util::has_ball(the_world, receiver);

		// kick it to a random place if the receiver has the ball
		if (receiverhasball) {
			std::cout << "strategy: receiver has ball, shoot randomly" << std::endl;

			kick kick_tactic(receiver, the_world);

			srand(time(NULL));
			double randX = ((rand() % 9)-4)/2.0;
			double randY = ((rand() % 9)-4)/2.0;
		
			point target(randX, randY);
			kick_tactic.set_target(target);
			kick_tactic.tick();
		}

		const player::ptr passer = the_team.get_player(0);
//		std::cout << passer->est_velocity() << std::endl;
		if (ai_util::has_ball(the_world, passer)) {
			std::cout << "strategy: passer has ball" << std::endl;
			//pass pass_tactic(passer, the_world, receiver);
			//pass_tactic.tick();
		} else if (receiverhasball) {
			// receiver has the ball, go somewhere?
		} else {
			std::cout << "strategy: chase ball" << std::endl;
			//chase chase_tactic(passer, the_world);
			//chase_tactic.tick();
		}
		pass pass_tactic(passer, the_world, receiver);
		pass_tactic.tick();
	}

	Gtk::Widget *test_pass_strategy::get_ui_controls() {
		return 0;
	}

	class test_pass_strategy_factory : public strategy_factory {
		public:
			test_pass_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	test_pass_strategy_factory::test_pass_strategy_factory() : strategy_factory("Test(Pass) Strategy") {
	}

	strategy::ptr test_pass_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new test_pass_strategy(world));
		return s;
	}

	test_pass_strategy_factory factory;

	strategy_factory &test_pass_strategy::get_factory() {
		return factory;
	}
}

