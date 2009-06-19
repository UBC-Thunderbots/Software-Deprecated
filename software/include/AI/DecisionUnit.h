//DecisionUnit: Top layer of strategy creation.
//Decides on the type of strategy that needs to be generated.
//Receives commands from the Referee Box.

#ifndef AI_DECISIONUNIT_H
#define AI_DECISIONUNIT_H

class AITeam;
class DecisionUnit {
public:	
	DecisionUnit(AITeam &team);
	void update();

private:
	DecisionUnit(const DecisionUnit &copyref); // Prohibit copying.
	AITeam &team;
};

#endif

