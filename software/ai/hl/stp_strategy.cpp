#include "ai/hl/strategy.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/dprint.h"
#include <glibmm.h>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using AI::HL::STP::Tactic::Tactic;

namespace {
	// The maximum amount of time a play can be running.
	const double PLAY_TIMEOUT = 30.0;

	/**
	 * STP based strategy.
	 * Please refer to the paper by CMU on
	 * STP: Skills, tactics and plays for multi-robot control in adversarial environments
	 */
	class STPStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			void initialize();

			void play();

			void on_player_added(std::size_t);
			void on_player_removed();

			/**
			 * When something bad happens,
			 * resets the current play.
			 */
			void reset();

			/**
			 * Calculates a NEW play to be used.
			 */
			void calc_play();

			/**
			 * Condition: a play is in use.
			 * Calculates and executes tactics.
			 */
			void execute_tactics();

			/**
			 * Condition: a valid list of tactics.
			 * Dynamically run tactic to play assignment.
			 */
			void role_assignment();

			static Strategy::Ptr create(AI::HL::W::World &world);

		private:
			bool initialized;

			Play::Ptr curr_play;

			// roles
			int curr_role_step;

			// the first role is for goalie
			std::vector<Tactic::Ptr> curr_roles[5];

			// active tactics
			Tactic::Ptr curr_active;
			Tactic::Ptr curr_tactics[5];

			// current player assignment
			Player::Ptr curr_assignment[5];

			// all the available plays
			std::vector<Play::Ptr> plays;

			STPStrategy(AI::HL::W::World &world);
			~STPStrategy();
	};

	/**
	 * A factory for constructing \ref STPStrategy "STPStrategies".
	 */
	class STPStrategyFactory : public StrategyFactory {
		public:
			STPStrategyFactory();
			~STPStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of STPStrategyFactory.
	 */
	STPStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		//		PlayType::PLAY,
	};

	STPStrategyFactory::STPStrategyFactory() : StrategyFactory("STP", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	STPStrategyFactory::~STPStrategyFactory() {
	}

	Strategy::Ptr STPStrategyFactory::create_strategy(World &world) const {
		return STPStrategy::create(world);
	}

	StrategyFactory &STPStrategy::factory() const {
		return factory_instance;
	}

	Strategy::Ptr STPStrategy::create(World &world) {
		const Strategy::Ptr p(new STPStrategy(world));
		return p;
	}

	void STPStrategy::reset() {
		curr_play.reset();
	}

	void STPStrategy::initialize() {
		if (initialized) return;
		const PlayFactory::Map &m = PlayFactory::all();
		for (PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
			plays.push_back(i->second->create(world));
		}
		initialized = true;
	}

	void STPStrategy::calc_play() {

		if (!initialized) {
			initialize();
		}

		// find a valid play
		std::random_shuffle(plays.begin(), plays.end());
		for (std::size_t i = 0; i < plays.size(); ++i) {
			if (plays[i]->applicable()) {
				// initialize this play and run it
				curr_play = plays[i];
				curr_play->initialize();
				curr_role_step = 0;
				for (std::size_t j = 0; j < 5; ++j) {
					curr_roles[j].clear();
				}
				curr_play->assign(curr_roles[0], curr_roles + 1);
				LOG_INFO(curr_play->factory().name());
				return;
			}
		}
	}

	void STPStrategy::role_assignment() {
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		int &r = curr_role_step;

		curr_active.reset();
		for (std::size_t i = 0; i < 5; ++i) {
			curr_tactics[i].reset();
		}

		for (std::size_t i = 0; i < 5; ++i) {
			if (curr_roles[i].size() <= r) {
				break;
			}

			curr_tactics[i] = curr_roles[i][r];

			if (curr_roles[i][r]->active()) {
				if (curr_active.is()) {
					LOG_ERROR("Multiple active tactics");
					reset();
					return;
				}
				curr_active = curr_roles[i][r];
			}
		}

		// no more active tactics
		if (!curr_active.is()) {
			LOG_ERROR("No active tactics");
			reset();
			return;
		}

		// do matching
		bool players_used[5];

		std::fill(players_used, players_used + 5, false);
		for (std::size_t i = 0; i < 5; ++i) {
			curr_assignment[i].reset();
		}

		for (std::size_t i = 0; i < 5; ++i) {
			if (!curr_tactics[i].is()) {
				break;
			}

			double best_score = 0;
			std::size_t best_j = 0;
			Player::Ptr best;
			for (std::size_t j = 0; j < players.size(); ++j) {
				if (players_used[j]) {
					continue;
				}
				double score = curr_tactics[i]->score(players[j]);
				if (!best.is() || score > best_score) {
					best = players[j];
					best_j = j;
					best_score = score;
				}
			}
			if (best.is()) {
				players_used[best_j] = true;
				curr_assignment[i] = best;
			}
		}

		bool active_assigned = false;
		for (std::size_t i = 0; i < 5; ++i) {
			if (!curr_assignment[i].is()) {
				continue;
			}
			curr_tactics[i]->set_player(curr_assignment[i]);
			if (curr_tactics[i]->active()) {
				active_assigned = true;
			}
		}

		// can't assign active tactic to anyone
		if (!active_assigned) {
			LOG_ERROR("Active tactic not assigned");
			reset();
			return;
		}
	}

	void STPStrategy::execute_tactics() {
		int &r = curr_role_step;
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		while (true) {
			role_assignment();

			if (!curr_play.is()) {
				return;
			}

			if (curr_active->done()) {
				++r;
				continue;
			}
		}

		// execute!
		for (std::size_t i = 0; i < 5; ++i) {
			if (!curr_assignment[i].is()) {
				continue;
			}
			curr_tactics[i]->tick();
		}
	}

	void STPStrategy::play() {

		// check if curr play wants to continue
		if (curr_play.is() && curr_play->done()) {
			curr_play.reset();
		}

		// check if curr is valid
		if (!curr_play.is()) {
			calc_play();
			if (!curr_play.is()) {
				LOG_WARN("calc play failed");
				return;
			}
		}

		execute_tactics();
	}

	void STPStrategy::on_player_added(std::size_t) {
		reset();
	}

	void STPStrategy::on_player_removed() {
		reset();
	}

	STPStrategy::STPStrategy(World &world) : Strategy(world), initialized(false) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &STPStrategy::on_player_added));
		world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &STPStrategy::on_player_removed));
	}

	STPStrategy::~STPStrategy() {
	}
}

