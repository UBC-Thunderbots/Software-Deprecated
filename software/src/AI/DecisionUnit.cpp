#include "AI/DecisionUnit.h"
#include "AI/Strategies/Strategy.h"
#include "AI/Strategies/OffensiveStrategy.h"
#include "AI/Strategies/DibsStrategy.h"
#include "AI/Strategies/DefenseStrategy.h"
#include "AI/Strategies/ChaseStrategy.h"
#include "AI/Strategies/CheaterStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

DecisionUnit::DecisionUnit(AITeam &team) : team(team) {
}

void DecisionUnit::update() {
	if (team.getCSU().strategy())
		return;
	PStrategy strat;
	if (team.index() == 0)
		strat.reset(new DibsStrategy(team));
	else
		strat.reset(new DibsStrategy(team));
	team.getCSU().strategy(strat);
}

