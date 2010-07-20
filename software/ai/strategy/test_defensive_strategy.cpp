#include "ai/strategy/strategy.h"
#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class TestDefensiveStrategy : public Strategy2 {
		public:
			TestDefensiveStrategy(RefPtr<World> world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const RefPtr<World> the_world;
	};

	TestDefensiveStrategy::TestDefensiveStrategy(RefPtr<World> world) : the_world(world) {

	}

	void TestDefensiveStrategy::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == PlayType::HALT) {
			return;
		}
		const FriendlyTeam &the_team(the_world->friendly);
		if (the_team.size() == 0) return;

		const RefPtr<Ball> the_ball(the_world->ball());
		Defensive defensive_role(the_world);
		Goalie goalie_role(the_world);
		std::vector<RefPtr<Player> > offenders;
		std::vector<RefPtr<Player> > goalies;

		goalies.push_back(the_team.get_player(0));

		for (size_t i = 1; i < the_team.size(); ++i) {
			offenders.push_back(the_team.get_player(i));
		}

		goalie_role.set_robots(goalies);
		defensive_role.set_robots(offenders);
		goalie_role.tick();
		defensive_role.tick();
	}

	Gtk::Widget *TestDefensiveStrategy::get_ui_controls() {
		return 0;
	}

	class TestDefensiveStrategyFactory : public StrategyFactory {
		public:
			TestDefensiveStrategyFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	TestDefensiveStrategyFactory::TestDefensiveStrategyFactory() : StrategyFactory("Test(Defensive & Goalie) Strategy") {
	}

	RefPtr<Strategy2> TestDefensiveStrategyFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new TestDefensiveStrategy(world));
		return s;
	}

	TestDefensiveStrategyFactory factory;

	StrategyFactory &TestDefensiveStrategy::get_factory() {
		return factory;
	}
}

