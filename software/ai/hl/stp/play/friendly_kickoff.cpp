#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/lone_goalie.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::W;

namespace {

	/**
	 * Condition:
	 * - It is the execute friendly kickoff play
	 *
	 * Objective:
	 * - Pass the ball to a friendly player without double touching the ball
	 */
	class FriendlyKickoff : public Play {
		public:
			FriendlyKickoff(World &world);
			~FriendlyKickoff();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<FriendlyKickoff> factory_instance("Execute Friendly Kickoff");

	const PlayFactory& FriendlyKickoff::factory() const {
		return factory_instance;
	}

	FriendlyKickoff::FriendlyKickoff(World &world) : Play(world) {
	}

	FriendlyKickoff::~FriendlyKickoff() {
	}

	bool FriendlyKickoff::applicable() const {
		return playtype(PlayType::EXECUTE_KICKOFF_FRIENDLY)->evaluate(world);
	}

	bool FriendlyKickoff::done() const {
		return !playtype(PlayType::EXECUTE_KICKOFF_FRIENDLY)->evaluate(world);
	}

	bool FriendlyKickoff::fail() const {
		return false;
	}

	void FriendlyKickoff::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		goalie_role.push_back(lone_goalie(world));

		/*
		roles[0].push_back();

		roles[1].push_back();

		roles[2].push_back();

		roles[3].push_back();
		*/
	}
}
