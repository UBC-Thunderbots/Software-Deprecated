#include "ai/strategy/basic_strategy.h"
#include "ai/tactic/chase.h"
#include "ai/role/defensive2.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/kickoff_friendly.h"             
#include "ai/role/penalty_friendly.h"
#include "ai/role/penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "ai/util.h"

#include <iostream>

namespace {

	/**
	 * A really basic Strategy that satisfies the rule.
	 * Most strategies should derive this function and overide in_play_assignment().
	 * Assumes that the first robot is always the goalie.
	 */
	class BetterStrategy : public BasicStrategy {
		public:
			BetterStrategy(World::ptr w);

			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();

		protected:

			void in_play_assignment();

			Player::ptr minus_one_assignment();
	};


	class BetterStrategyFactory : public StrategyFactory {
		public:
			BetterStrategyFactory();
			Strategy::ptr create_strategy(World::ptr world);
	};

	BetterStrategyFactory::BetterStrategyFactory() : StrategyFactory("Better than Basic Strategy") {
	}

	Strategy::ptr BetterStrategyFactory::create_strategy(World::ptr world) {
		Strategy::ptr s(new BetterStrategy(world));
		return s;
	}

	BetterStrategyFactory factory;

	BetterStrategy::BetterStrategy(World::ptr w) : BasicStrategy(w) {
	}

	Gtk::Widget *BetterStrategy::get_ui_controls() {
		return NULL;
	}

	void BetterStrategy::in_play_assignment() {
		const FriendlyTeam &the_team(world->friendly);

		// TODO: SORT

		roles.clear();
		if (the_team.size() == 0) return;

		// const vector<Player::ptr> players = the_team.get_players();

		Defensive2::ptr defensive_role(new Defensive2(world));
		Offensive::ptr offensive_role(new Offensive(world));
		roles.push_back(Role::ptr(defensive_role));
		roles.push_back(Role::ptr(offensive_role));
		std::vector<Player::ptr> defenders;
		std::vector<Player::ptr> offenders;

		defenders.push_back(the_team.get_player(0));

		if (the_team.size() >= 2)
			offenders.push_back(the_team.get_player(1));

		if (the_team.size() >= 3)
			defenders.push_back(the_team.get_player(2));

		if (the_team.size() >= 4){
			switch(world->playtype()){
				case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
				case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
				case PlayType::PREPARE_KICKOFF_ENEMY:
				case PlayType::EXECUTE_KICKOFF_ENEMY:
				case PlayType::PREPARE_PENALTY_ENEMY:
				case PlayType::EXECUTE_PENALTY_ENEMY:
					defenders.push_back(the_team.get_player(3));
					break;
				default:
					offenders.push_back(the_team.get_player(3));
			}
		}

		// 3 offenders, 1 defender if we have the ball
		// otherwise 2 offenders, 2 defenders
		if (the_team.size() >= 5){
			if (AIUtil::friendly_posses_ball(world))
				offenders.push_back(the_team.get_player(4));
			else
				defenders.push_back(the_team.get_player(4));
		}

		// If we have ball and ball is on non-goalie defender, switch this defender to offender
		// and switch robot 4 back to defender
		if (defenders.size() >= 2 && AIUtil::posses_ball(world,defenders[1]))
			std::swap(defenders[1],offenders[offenders.size()-1]);
		// extra players become offenders
		for (size_t i = 5; i < the_team.size(); ++i)
			offenders.push_back(the_team.get_player(i));

		defensive_role->set_players(defenders);
		defensive_role->set_goalie(the_team.get_player(0));
		offensive_role->set_players(offenders);
	}

	Player::ptr BetterStrategy::minus_one_assignment() {

		// TODO: SORT

		const FriendlyTeam &the_team(world->friendly);

		roles.clear();
		if (the_team.size() == 0) return Player::ptr();

		if (the_team.size() == 1) return the_team.get_player(0);

		// other players just sort by distance

		Defensive2::ptr defensive_role(new Defensive2(world));
		Offensive::ptr offensive_role(new Offensive(world));
		roles.push_back(Role::ptr(defensive_role));
		roles.push_back(Role::ptr(offensive_role));
		std::vector<Player::ptr> defenders;
		std::vector<Player::ptr> offenders;

		defenders.push_back(the_team.get_player(0));
		if (the_team.size() >= 3)
			defenders.push_back(the_team.get_player(2));

		if (the_team.size() >= 4)
			offenders.push_back(the_team.get_player(3));

		// 3 offenders, 1 defender if we have the ball
		// otherwise 2 offenders, 2 defenders
		if (the_team.size() >= 5) {
			if (AIUtil::friendly_posses_ball(world))
				offenders.push_back(the_team.get_player(4));
			else
				defenders.push_back(the_team.get_player(4));
		}

		// extra players become offenders
		for (size_t i = 5; i < the_team.size(); ++i)
			offenders.push_back(the_team.get_player(i));

		defensive_role->set_players(defenders);
		defensive_role->set_goalie(the_team.get_player(0));
		offensive_role->set_players(offenders);

		return the_team.get_player(1);
	}

	StrategyFactory &BetterStrategy::get_factory() {
		return factory;
	}

}

