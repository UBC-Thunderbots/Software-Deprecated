#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"

#include "ai/hl/stp/evaluation/enemy.h"

#include <set>

using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Evaluation::EnemyThreat;

namespace{
	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/predicates", 3.0, 1.0, 10.0);

	Player::CPtr calc_baller(const World &world){
		const FriendlyTeam &friendly = world.friendly_team();
		std::set<Player::CPtr> players;
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			players.insert(friendly.get(i));
		}
		const Player::CPtr baller = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::CPtr>(world.ball().position()));
		return baller;
	}
	
	const Robot::Ptr calc_enemy_baller(const World &world){
		const EnemyTeam &enemy = world.enemy_team();
		std::set<Robot::Ptr> enemies;
		for (std::size_t i = 0; i < enemy.size(); ++i) {
			enemies.insert(enemy.get(i));
		}
		const Robot::Ptr baller = *std::min_element(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));
		return baller;
	}
}

bool AI::HL::STP::Predicates::goal(const World &) {
	return false;
}

bool AI::HL::STP::Predicates::playtype(const World &world, AI::Common::PlayType playtype) {
	return world.playtype() == playtype;
}

bool AI::HL::STP::Predicates::our_ball(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i)->has_ball()) {
			return true;
		}
	}
	return false;
}

bool AI::HL::STP::Predicates::their_ball(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
			return true;
		}
	}
	return false;
}

bool AI::HL::STP::Predicates::none_ball(const World &world) {
	return !our_ball(world) && !their_ball(world);
}

bool AI::HL::STP::Predicates::our_team_size_at_least(const World &world, const unsigned int n) {
	return world.friendly_team().size() >= n;
}

bool AI::HL::STP::Predicates::our_team_size_exactly(const World &world, const unsigned int n) {
	return world.friendly_team().size() == n;
}

bool AI::HL::STP::Predicates::their_team_size_at_least(const World &world, const unsigned int n) {
	return world.enemy_team().size() >= n;
}

bool AI::HL::STP::Predicates::their_team_size_at_most(const World &world, const unsigned int n) {
	return world.enemy_team().size() <= n;
}

bool AI::HL::STP::Predicates::ball_x_less_than(const World &world, const double x) {
	return world.ball().position().x < x;
}

bool AI::HL::STP::Predicates::ball_x_greater_than(const World &world, const double x) {
	return world.ball().position().x > x;
}

bool AI::HL::STP::Predicates::ball_on_our_side(const World &world) {
	return world.ball().position().x <= 0;
}

bool AI::HL::STP::Predicates::ball_on_their_side(const World &world) {
	return world.ball().position().x > 0;
}

bool AI::HL::STP::Predicates::ball_in_our_corner(const World &world) {
	return world.ball().position().x <= -world.field().length()/4 && std::fabs(world.ball().position().y) > world.field().goal_width();
}

bool AI::HL::STP::Predicates::ball_in_their_corner(const World &world) {
	return world.ball().position().x >= world.field().length()/4 && std::fabs(world.ball().position().y) > world.field().goal_width();
}

bool AI::HL::STP::Predicates::ball_midfield(const World &world){
	return std::fabs(world.ball().position().x) < world.field().length()/4;
}

bool AI::HL::STP::Predicates::baller_can_shoot(const World &world){
	const Player::CPtr baller = calc_baller(world);
	
	return AI::HL::Util::calc_best_shot(world, baller).second > AI::HL::Util::shoot_accuracy * M_PI / 180.0;
}

bool AI::HL::STP::Predicates::baller_can_pass(const World &world){
	
	// should write a player evaluation layer to handle this
	
	const std::vector<Player::CPtr> players = AI::HL::Util::get_players(world.friendly_team());
	/*
	const std::vector<Player::CPtr> supporters;
	// don't count in the goalie
	for (size_t i = 1; i < players.size(); ++i) {
		if (world.friendly_team().get(i)->has_ball()) continue;
		supporters.push_back(players[i]);
	}
	
	Player::CPtr passee = AI::HL::Util::choose_best_pass(world, supporters);
	*/
	return false;
}

bool AI::HL::STP::Predicates::baller_can_shoot_target(const World &world, const Point &target){
	const Player::CPtr baller = calc_baller(world);
	
	return AI::HL::Util::calc_best_shot_target(world, target, baller).second > AI::HL::Util::shoot_accuracy * M_PI / 180.0;
}

bool AI::HL::STP::Predicates::baller_under_threat(const World &world){

	const Player::CPtr baller = calc_baller(world);
	
	int enemy_cnt = 0;
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {	
		if ((baller->position() - enemies.get(i)->position()).len() <= near_thresh * AI::HL::W::Robot::MAX_RADIUS)
			enemy_cnt++;
	}
	
	return enemy_cnt >= 2;
}

bool AI::HL::STP::Predicates::enemy_baller_can_shoot(const World &world){
	const Robot::Ptr baller = calc_enemy_baller(world);
	
	return Evaluation::eval_enemy(world, baller).passes == 0;
}

bool AI::HL::STP::Predicates::enemy_baller_can_pass(const World &world){
	const Robot::Ptr baller = calc_enemy_baller(world);
	
	return Evaluation::eval_enemy(world, baller).passees.size() > 0;
}

bool AI::HL::STP::Predicates::enemy_baller_can_pass_shoot(const World &world){
	const Robot::Ptr baller = calc_enemy_baller(world);
	
	return Evaluation::eval_enemy(world, baller).passes > 0 && Evaluation::eval_enemy(world, baller).passes < 3;
}

