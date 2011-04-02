#include "ai/flags.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::chase(const World &world, Player::Ptr player,  const unsigned int flags) {
	chase(world, player, world.ball().position(),flags);
}

void AI::HL::STP::Action::chase(const World &world, Player::Ptr player, Point target, const unsigned int flags) {
	player->move(world.ball().position(), (target - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
}

void AI::HL::STP::Action::repel(const World &world, Player::Ptr player, const unsigned int flags) {
	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);
		return;
	}

	// just shoot as long as it's not in backwards direction
	if (player->orientation() < M_PI / 2 && player->orientation() > -M_PI / 2) {
		// should autokick??
		// player->autokick(10.0); 
		if (player->chicker_ready()) {
			player->kick(10.0);
		}
	}

	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);

	// all enemies are obstacles
	/*
	   std::vector<Point> obstacles;
	   EnemyTeam &enemy = world.enemy_team();
	   for (std::size_t i = 0; i < enemy.size(); ++i) {
	   obstacles.push_back(enemy.get(i)->position());
	   }

	   const Field &f = world.field();

	   // vertical line at the enemy goal area
	   // basically u want the ball to be somewhere there
	   const Point p1 = Point(f.length() / 2.0, -f.width() / 2.0);
	   const Point p2 = Point(f.length() / 2.0, f.width() / 2.0);
	   std::pair<Point, double> target = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	   AI::HL::STP::Action::shoot(world, player, flags, target.first);
	 */
}

void AI::HL::STP::Action::free_move(const World &world, Player::Ptr player, const Point p) {
	player->move(p, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
}

void AI::HL::STP::Action::block(const World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot) {
	
	// should have threshold distance, half a robot radius?
	
	//Point near_enemy(enemy->evaluate()->position().x - Robot::MAX_RADIUS * 3, enemy->evaluate()->position().y);
	//player->move(near_enemy, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);

	Point dirToGoal = (world.field().friendly_goal() - robot->position()).norm();
	player->move(robot->position() + (0.5*Robot::MAX_RADIUS*dirToGoal), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

void AI::HL::STP::Action::block_pass(const World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot) {

	// should have threshold distance, half a robot radius?
	// TODO: Use this somehow
	Point dirToBall = (world.ball().position() - robot->position()).norm();
	player->move(robot->position() + (0.5*Robot::MAX_RADIUS*dirToBall), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

