#include "ai/strategy/movement_benchmark.h"
#include "robot_controller/tunable_controller.h"
#include "util/stochastic_local_search.h"
#include <iostream>
#include <vector>
#include <cmath>

// Parameter Tuning based on movement benchmark
// To change tasks, make use of MovementBenchmark

namespace {

	const int TUNING_ITERATIONS = 1000;
	const int EVALUATION_LIMIT = 1500;

	class ParamTuning : public MovementBenchmark {
		public:
			ParamTuning(RefPtr<World> world);
			~ParamTuning();
			Gtk::Widget *get_ui_controls();
			StrategyFactory &get_factory();
			void tick();
			void reset();
			void hillclimb();
			void revert();
		private:
			const RefPtr<World> the_world;
			Gtk::Button revert_button;
			Gtk::Button reset_button;
			Gtk::Button hillclimb_button;
			Gtk::VBox vbox;
			StochasticLocalSearch* sls;
			TunableController* tc;
			int sls_counter;
			int best;
	};

	ParamTuning::ParamTuning(RefPtr<World> world) : MovementBenchmark(world), the_world(world), revert_button("Redo from last best"), reset_button("Complete Reset"), hillclimb_button("Hill Climb Again"), sls(0) {
		sls_counter = 0;

		// override the reset button
		revert_button.signal_clicked().connect(sigc::mem_fun(this,&ParamTuning::revert));
		reset_button.signal_clicked().connect(sigc::mem_fun(this,&ParamTuning::reset));
		hillclimb_button.signal_clicked().connect(sigc::mem_fun(this,&ParamTuning::hillclimb));
		best = EVALUATION_LIMIT;
		vbox.add(revert_button);
		vbox.add(reset_button);
		vbox.add(hillclimb_button);
	}

	ParamTuning::~ParamTuning() {
		if(sls != NULL) delete sls;
	}

	Gtk::Widget *ParamTuning::get_ui_controls() {
		return &vbox;
	}

	void ParamTuning::reset() {
		if (sls != NULL) delete sls;
		tc = TunableController::get_instance();
		if (tc == NULL) return;
		sls = new StochasticLocalSearch(tc->get_params_default(), tc->get_params_min(), tc->get_params_max());
		done = 0;
		time_steps = 0;
		best = EVALUATION_LIMIT;
		sls_counter = 0;
		tc->set_params(sls->get_best_params());
		std::cout << " reset, curr params=";
		const std::vector<double> params = sls->get_best_params();
		for (unsigned int i = 0; i < params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void ParamTuning::hillclimb() {
		sls->revert();
		sls->hill_climb();
		tc->set_params(sls->get_params());
		done = 0;
		time_steps = 0;
		std::cout << " hill climb, curr params=";
		const std::vector<double> params = sls->get_params();
		for (unsigned int i = 0; i < params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void ParamTuning::revert() {
		sls->revert();
		tc->set_params(sls->get_params());
		done = 0;
		time_steps = 0;
		std::cout << " revert curr params=";
		const std::vector<double> params = sls->get_params();
		for (unsigned int i = 0; i < params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void ParamTuning::tick() {
		if (tc == NULL || tc != TunableController::get_instance()) {
			reset();
			return;
		}
		// std::cout << " tick " << std::endl;
		if (sls_counter > TUNING_ITERATIONS) {
			// done with sls
			return;
		}
		if (the_world->friendly.size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}
		if (done >= tasks.size() || time_steps > best) {
			if (time_steps < best) {
				std::cout << "good, new params" << std::endl;
				best = time_steps;
				sls->set_cost(time_steps);
				sls->hill_climb();
			} else {
				sls->set_cost(EVALUATION_LIMIT);
				sls->hill_climb();
			}
			tc->set_params(sls->get_params());
			std::cout << "curr params=";
			const std::vector<double> params = sls->get_params();
			for (unsigned int i = 0; i < params.size(); ++i) {
				std::cout << params[i] << " ";
			}
			std::cout << std::endl;
			const std::vector<double> best_params = sls->get_best_params();
			std::cout << "best params=";
			for (unsigned int i = 0; i < best_params.size(); ++i) {
				std::cout << best_params[i] << " ";
			}
			std::cout << std::endl;
			sls_counter++;
			done = 0;
			time_steps = 0;
		}
		MovementBenchmark::tick();
	}

	class ParamTuningFactory : public StrategyFactory {
		public:
			ParamTuningFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	ParamTuningFactory::ParamTuningFactory() : StrategyFactory("Param Tuning") {
	}

	RefPtr<Strategy2> ParamTuningFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new ParamTuning(world));
		return s;
	}

	ParamTuningFactory factory;

	StrategyFactory &ParamTuning::get_factory() {
		return factory;
	}

}
