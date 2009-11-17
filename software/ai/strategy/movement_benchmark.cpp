#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/move.h"
#include <iostream>

// This benchmark records how long it takes for a robot to travel and stop at a point 1 meter away.

namespace {
	class movement_benchmark : public strategy {
		public:
			movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);
		private:
			point destination;
			int time_steps;
			bool done;
			double dis_threshold;
			double vel_threshold;
	};

	movement_benchmark::movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {
		destination = team->get_player(0)->position();
		destination.x += 1;
		time_steps = 0;
		done = false;
		dis_threshold = 1e-2;
		vel_threshold = 1e-4;
	}

	void movement_benchmark::tick() {
		if (!done) {
			time_steps++;
			point diff = the_team->get_player(0)->position() - destination;
			point vel = the_team->get_player(0)->est_velocity();
			std::cout << "position: " << diff.x << " " << diff.y << std::endl;
			std::cout << "velocity: " << the_team->get_player(0)->est_velocity().x << " " << the_team->get_player(0)->est_velocity().y << std::endl;
			if (diff.len() < dis_threshold && vel.len() < vel_threshold) {
				std::cout << "time steps taken: " << time_steps << std::endl;
				done = true;
			}
		}
		move::ptr mover (new move(the_ball, the_field, the_team, the_team->get_player(0)));
		mover->set_position(destination);
		mover->tick();
	}

	void movement_benchmark::set_playtype(playtype::playtype) {
	}
	
	Gtk::Widget *movement_benchmark::get_ui_controls() {
		return 0;
	}

	void movement_benchmark::robot_added(void){
	}

	void movement_benchmark::robot_removed(unsigned int index, robot::ptr r){
	}

	class movement_benchmark_factory : public strategy_factory {
		public:
			movement_benchmark_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	movement_benchmark_factory::movement_benchmark_factory() : strategy_factory("Movement Benchmark") {
	}

	strategy::ptr movement_benchmark_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new movement_benchmark(ball, field, team, pt_src));
		return s;
	}

	movement_benchmark_factory factory;

	strategy_factory &movement_benchmark::get_factory() {
		return factory;
	}
}

