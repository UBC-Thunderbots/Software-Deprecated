#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Skill;
using AI::HL::STP::SSM::SkillStateMachine;
using namespace AI::HL::W;

ContextImpl::ContextImpl(World& w, Param& p) : world(w), param(p), ssm(NULL), next_skill(NULL) {
}

void ContextImpl::set_player(Player::Ptr p) {
	player = p;
}

void ContextImpl::set_ssm(const AI::HL::STP::SSM::SkillStateMachine* s) {
	if (ssm != s ){
		ssm = s;
		if (ssm == NULL) {
			next_skill = NULL;
		} else {
			next_skill = ssm->initial();
		}
	}
}

void ContextImpl::reset_ssm() {
	if (ssm != NULL) {
		next_skill = ssm->initial();
	}
}

bool ContextImpl::done() const {
	return ssm == NULL;
}

void ContextImpl::run() {
	if (next_skill == NULL) {
		// ???
		return;
	}

	do {
		execute_next_skill = false;

		if (history.find(next_skill) != history.end()) {
			LOG_ERROR("Skill loop!");
			break;
		}

		history.insert(next_skill);
		next_skill->execute(world, player, ssm, param, *this);
	} while (execute_next_skill);
	history.clear();
}

void ContextImpl::execute_after(const Skill* skill) {
	next_skill = skill;
	execute_next_skill = true;
}

void ContextImpl::finish() {
	ssm = NULL;
}

void ContextImpl::abort() {
#warning TODO
}

void ContextImpl::transition(const Skill* skill) {
	next_skill = skill;
}

