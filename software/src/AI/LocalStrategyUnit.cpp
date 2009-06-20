#include "AI/LocalStrategyUnit.h"
#include "AI/AITeam.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/RobotController.h"
#include "datapool/Player.h"
#include "datapool/PlayType.h"
#include "datapool/Vector2.h"
#include "datapool/World.h"

#include <cstdlib>
#include <cmath>
#include <stdint.h>

LocalStrategyUnit::LocalStrategyUnit(AITeam &team) : team(team) {
}

void LocalStrategyUnit::update() {
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		PPlayer player = team.player(i);
		if (player->receivingPass())
			passee(player);
		else
			switch (player->plan()) {
				case Plan::stop:
					stop(player);
					break;

				case Plan::passer:
					pass(player);
					break;

				case Plan::shoot:
					shoot(player);
					break;

				case Plan::chase:
					chaseBall(player);
					break;

				case Plan::move:
					move(player, player->destination());
					break;

				case Plan::goalie:
					goalie(player);
					break;

			}
	}
}

void LocalStrategyUnit::stop(PPlayer robot) {
	// Always orient towards the ball:
	Vector2 orientation = World::get().ball().position() - robot->position();
	double angle = orientation.angle();
	RobotController::sendCommand(robot, Vector2(0,0), angle, 255, 0);
}

void LocalStrategyUnit::pass(PPlayer passer) {
	PPlayer passee = passer->otherPlayer();
	if (!passer->hasBall()) {
		// If the passer does not have possession of the ball, chase the ball.
		chaseBall(passer);
		return;
	}

	if (passee->velocity().length() > 0.5) { // Wait until the passee is not moving:
		Vector2 vec = passee->position() - passer->position();
		double angle = vec.angle();
		// Orient towards the passee:
		RobotController::sendCommand(passer, Vector2(0, 0), angle, 255, 0);
		return;	
	}

	Vector2 des = passee->position();
	Vector2 pos = passer->position();

	if (!CentralAnalyzingUnit::checkVector(pos, des, passer, 1, passee)) {
		// The path is clear, so pass the ball:
		double angle = (des - pos).angle();
		RobotController::sendCommand(passer, Vector2(0, 0), angle, 255, 255);
	} else {
		// Find a clear path:
		move(passer, des);
	}
}

void LocalStrategyUnit::passee(PPlayer robot) {
	if (robot->hasBall()) {
		robot->receivingPass(false);
	} else {
		// Make sure the passer has the ball:
		bool possessed = false;
		for (unsigned int i = 0; i < Team::SIZE * 2; i++) {
			if (World::get().player(i)->hasBall()) {
				possessed = true;
				if (World::get().player(i)->plan() != Plan::passer) {
					// Passer no longer has the ball, so pass unsuccessful:
					robot->receivingPass(false);
				}
			}
		}
		if (!possessed) {
			// Make sure the ball is headed towards the passee:
			Vector2 diff = robot->position() - World::get().ball().position();
			Vector2 vel = World::get().ball().velocity();

			// If the ball is not moving, pass unsuccessful:
			if (vel.length() == 0)
				robot->receivingPass(false);

			vel *= 1 / vel.length();
			diff *= 1 / diff.length();
			diff -= vel;

			if (diff.length() > 0.1) {
				// The ball is not moving towards the robot, so pass unsuccessful:
				robot->receivingPass(false);
			}
		}
	}
	stop(robot);
}

void LocalStrategyUnit::chaseBall(PPlayer robot) {
	if (robot->hasBall()) {
		// If the robot already has the ball, don't chase it!
		shoot(robot);
	} else {
		// Chases ball, using move function
		
		Vector2 v = World::get().ball().position() - robot->position();
		Vector2 m_v = v - Vector2(v.angle()) * World::get().field()->convertMmToCoord(80);
		
		move(robot, robot->position() + m_v);
		move(robot, World::get().ball().position());
	}
}

void LocalStrategyUnit::move(PPlayer robot, Vector2 pos, double speed) {
	// Do not go outside the bounds of the field:
	PField field = World::get().field();
	if (pos.x > field->east())
		pos.x = field->east();
	if (pos.x < field->west())
		pos.x = field->west();
	if (pos.y > field->south())
		pos.y = field->south();
	if (pos.y < field->north())
		pos.y = field->north();

	// If the robot has the ball, stay even further from the boundary:
	if (robot->hasBall()) {
		if (pos.x > field->east()  - field->width() / 20.0)
			pos.x = field->east()  - field->width() / 20.0;
		if (pos.x < field->west()  + field->width() / 20.0)
			pos.x = field->west()  + field->width() / 20.0;
		if (pos.y > field->south() - field->width() / 20.0)
			pos.y = field->south() - field->width() / 20.0;
		if (pos.y < field->north() + field->width() / 20.0)
			pos.y = field->north() + field->width() / 20.0;
	}

	Vector2 curPos = robot->position();
	Vector2 curVel = robot->velocity();
	Vector2 diff = pos - curPos;
	
	pos = curPos + diff;

	// Create a buffer zone to stop the robot:
	if (diff.length() < field->convertMmToCoord(2000)) {
		diff /= field->convertMmToCoord(2000);
	} else {
		diff /= diff.length();
	}

	// Avoid obstacles if not goalie:
	//bool west = team.side();
	//double fuzzyFactor = curPos.x / field->width();
	Vector2 velPosL;
	Vector2 velPosR;
	
	Vector2 diffL = Vector2(diff.angle()) * diff.length();	
	Vector2 diffR = Vector2(diff.angle()) * diff.length();
	
	velPosL = curPos + diff * field->convertMmToCoord(2000);
	velPosR = curPos + diff * field->convertMmToCoord(2000);
	
	int maxTurn = 360;
	bool avoid = false;
	
	int avoidOffset = 0;
	
	bool probRight;
	bool probLeft;
	
	while (((probRight = CentralAnalyzingUnit::checkVector(curPos, velPosR, robot, 1)) && (probLeft = CentralAnalyzingUnit::checkVector(curPos, velPosL, robot, 1))) && maxTurn > 0) {
		avoid = true;
		// Move around the obstacle by going left:
		diffL = Vector2(diffL.angle() + 5) * diffL.length();
		diffR = Vector2(diffR.angle() - 5) * diffR.length();
		velPosL = curPos + diffL * field->convertMmToCoord(2000);
		velPosR = curPos + diffR * field->convertMmToCoord(2000);
		maxTurn -= 5;
	}
	if (avoid) {
		if (!probRight) {
			diff = diffR;
			avoidOffset = -45;
		} else if (!probLeft) {
			diff = diffL;
			avoidOffset = 45;
		}
		diff = Vector2(diff.angle() + avoidOffset) * diff.length();
	}
	
	uint8_t dribble = 255;

	if (!robot->allowedInside()) {
		dribble = 0;
		// The robots can not move within 500mm of the ball when it is not play mode.
		pos = curPos + diff;
		Vector2 ballPos = World::get().ball().position();
		Vector2 ballDis = curPos - ballPos;
		double boundary = field->convertMmToCoord(800);
		if (ballDis.length() < boundary) {
			Vector2 interceptPoint = CentralAnalyzingUnit::lcIntersection(curPos,pos,ballPos,boundary);
			diff = interceptPoint - pos;
			
			// Create a buffer zone to stop the robot:
			if (diff.length() < field->convertMmToCoord(1500)) {
				diff /= field->convertMmToCoord(1500);
			} else {
				diff /= diff.length();
			}
		}
	}

	double angle = robot->orientation();

	if (robot->plan() == Plan::goalie && robot->hasBall()) {
		// Orient towards the closest friendly player:
		PPlayer friendly = CentralAnalyzingUnit::closestRobot(robot, CentralAnalyzingUnit::TEAM_SAME, false);
		Vector2 vec = friendly->position() - robot->position();
		angle = vec.angle();
	} else if (robot->hasBall()) {
		// Orient towards the destination. If the robot's orientation is not correct, pivot around the ball.
		angle = diff.angle();
		if (std::fabs(robot->orientation() - angle) > 5)
			diff = Vector2(robot->orientation() + 90);
	} else {
		// Always orient towards the ball:
		Vector2 orientation = World::get().ball().position() - robot->position();
		angle = orientation.angle();
	}

	RobotController::sendCommand(robot, diff, angle, dribble, 0);
}

void LocalStrategyUnit::shoot(PPlayer robot) {
	if (!robot->hasBall()) {
		// If the robot does not have possession of the ball, chase the ball.
		chaseBall(robot);
		return;
	}

	bool west = team.side();

	PField field = World::get().field();

	Vector2 des;
	Vector2 pos = robot->position();

	if (!west)
		des = Vector2(field->west(), (field->westGoal()->south.y + field->westGoal()->north.y) / 2.0);
	else
		des = Vector2(field->east(), (field->westGoal()->south.y + field->westGoal()->north.y) / 2.0);

	// Check the vector between the robot and the center of the goal:
	if (CentralAnalyzingUnit::checkVector(pos, des, robot, 0)) {
		if (team.other().player(0)->position().y > field->centerCircle().y) {
			// The shot is not clear, so check a point further north:
			double height = field->eastGoal()->south.y - field->eastGoal()->north.y;
			des.y -= height / 3.0;
			if (CentralAnalyzingUnit::checkVector(pos, des, robot, 0)) {
				// The shot is not clear, so check a point further south:
				des.y += 2.0 * height / 3.0;
				if (CentralAnalyzingUnit::checkVector(pos, des, robot, 0)) {
					// The shot is still not clear, so move to a clear shot:
					des.y -= height / 3.0;
					move(robot, des);
					return;
				}
			}
		} else {
			// The shot is not clear, so check a point further south:
			double height = field->eastGoal()->south.y - field->eastGoal()->north.y;
			des.y += height / 3.0;
			if (CentralAnalyzingUnit::checkVector(pos, des, robot, 0)) {
				// The shot is not clear, so check a point further north:
				des.y -= 2.0 * height / 3.0;
				if (CentralAnalyzingUnit::checkVector(pos, des, robot, 0)) {
					// The shot is still not clear, so move to a clear shot:
					des.y += height / 3.0;
					move(robot, des);
					return;
				}
			}
		}
	}

	// A clear shot has been found:
	Vector2 orientation = des - pos;

	if (orientation.length() > field->width() / 5.0) {
		move(robot, des); // Goal is too far away, so move closer.
		return;
	}

	double angle = orientation.angle();
	
	if (std::fabs(robot->orientation() - angle) > 5) {
		// Get into the correct orientation for the shot:
		move(robot, des); // Goal is too far away, so move closer.
		return;
	}
	
	RobotController::sendCommand(robot, Vector2(0, 0), angle, 255, 255);
}

void LocalStrategyUnit::goalie(PPlayer robot) {
	bool west = team.side();

	PField field = World::get().field();
	Vector2 pos = World::get().ball().position();
	double rad; // The size of the goal.
	if (west)
		rad = field->westGoal()->south.y - field->westGoal()->north.y;
	else
		rad = field->eastGoal()->south.y - field->eastGoal()->north.y;

	rad *= 0.5;

	Vector2 center; // The center position of the goal.
	if (west)
		center = Vector2(field->west(), (field->westGoal()->south.y + field->westGoal()->north.y) / 2.0);
	else
		center = Vector2(field->east(), (field->westGoal()->south.y + field->westGoal()->north.y) / 2.0);

	Vector2 vec = pos - center; // Vector between the ball and the goal.

	Vector2 target = World::get().ball().velocity();
	target *= (1.0 / target.length()); // get the unit vector.
	target *= vec.length();
	if (target.x != 0 && target.y != 0)
		target += pos;

	if (target.y < field->eastGoal()->south.y && 
			target.y > field->eastGoal()->north.y) {
		// If the ball is headed towards the goal, change the "center" of the goal to the ball's path.
		center.y = target.y;
	}

	// Find the destination point for the goalie to move towards.
	double R = field->width() / 2.0;
	vec = pos - center;
	Vector2 des;
	if (vec.length() <= 3.0 * rad) {
		des = center + 2.0 * vec * (rad / vec.length());
	} else {
		des = center + 2.0 * vec * (rad/ R);
	}
	if (World::get().playType() == PlayType::preparePenaltyKick || World::get().playType() == PlayType::penaltyKick){
		if (team.side()){
			double maxx = field->west() + robot->radius() / 2;
			if(des.x > maxx)
				des.x = maxx;
		}
		else{
			double minx = field->east() - robot->radius() / 2;
			if(des.x < minx)
				des.x = minx;
		}
		robot->hasBall(false);
	}

	move(robot, des);
}

