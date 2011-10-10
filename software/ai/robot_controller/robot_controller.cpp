#include "ai/robot_controller/robot_controller.h"
#include "util/param.h"

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::OldRobotController2;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

void RobotController::convert_to_wheels(const Point &vel, Angle avel, int(&wheel_speeds)[4]) {
	static const double WHEEL_MATRIX[4][3] = {
		{ -42.5995, 27.6645, 4.3175 },
		{ -35.9169, -35.9169, 4.3175 },
		{ 35.9169, -35.9169, 4.3175 },
		{ 42.5995, 27.6645, 4.3175 }
	};
	const double input[3] = { vel.x, vel.y, avel.to_radians() };
	double output[4] = { 0, 0, 0, 0 };
	for (unsigned int row = 0; row < 4; ++row) {
		for (unsigned int col = 0; col < 3; ++col) {
			output[row] += WHEEL_MATRIX[row][col] * input[col];
		}
	}
	for (unsigned int row = 0; row < 4; ++row) {
		wheel_speeds[row] = static_cast<int>(output[row]);
	}
}

void RobotController::draw_overlay(Cairo::RefPtr<Cairo::Context>) {
}

RobotController::RobotController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player) : world(world), player(player) {
}

RobotController::~RobotController() = default;

OldRobotController2::OldRobotController2(AI::RC::W::World &world, AI::RC::W::Player::Ptr player) : RobotController(world, player) {
}

OldRobotController2::~OldRobotController2() = default;

void OldRobotController2::tick() {
	const AI::RC::W::Player::Path &path = player->path();
	if (path.empty()) {
		clear();
	} else {
		int wheels[4];
		move(path[0].first.first, path[0].first.second, wheels);
		player->drive(wheels);
	}
}

void OldRobotController::move(const Point &new_position, Angle new_orientation, int(&wheel_speeds)[4]) {
	Point vel;
	Angle avel;
	move(new_position, new_orientation, vel, avel);
	convert_to_wheels(vel, avel, wheel_speeds);
}

OldRobotController::OldRobotController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player) : OldRobotController2(world, player) {
}

OldRobotController::~OldRobotController() = default;

Gtk::Widget *RobotControllerFactory::ui_controls() {
	return 0;
}

RobotControllerFactory::RobotControllerFactory(const char *name) : Registerable<RobotControllerFactory>(name) {
}

