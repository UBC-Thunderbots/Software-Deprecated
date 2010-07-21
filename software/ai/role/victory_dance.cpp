#include "ai/role/victory_dance.h"

VictoryDance::VictoryDance() {
}

void VictoryDance::tick(){
	for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
    }
}

void VictoryDance::players_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<players.size(); i++) {
        Dance::Ptr tactic( new Dance(players[i]));
        the_tactics.push_back(tactic);
    }
}
