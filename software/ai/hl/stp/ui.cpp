#include "ai/hl/stp/ui.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include <cmath>

using namespace AI::HL::STP;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Evaluation::grid_x;
using AI::HL::STP::Evaluation::grid_y;

void AI::HL::STP::draw_shoot(const World &world, Cairo::RefPtr<Cairo::Context> ctx) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr player = friendly.get(i);
		Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

		// draw yellow circle
		if (shoot_data.can_shoot) {
			ctx->set_source_rgba(1.0, 1.0, 1.0, 0.5);
			ctx->arc(player->position().x, player->position().y, 0.06, 0.0, 2 * M_PI);
			ctx->fill();
			ctx->stroke();
		}
	}
}

void AI::HL::STP::draw_offense(const World &world, Cairo::RefPtr<Cairo::Context> ctx) {
	// draw yellow circles for shooting
	/*
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr player = friendly.get(i);
		std::pair<Point, double> best_shot = AI::HL::Util::calc_best_shot(world, player);
		if (best_shot.second < AI::HL::Util::shoot_accuracy * M_PI / 180) {
			continue;
		}

		const double radius = best_shot.second * 0.5;

		// draw yellow circle
		ctx->set_source_rgba(1.0, 1.0, 0.5, 0.2);
		ctx->arc(player->position().x, player->position().y, radius, 0.0, 2 * M_PI);
		ctx->fill();
		ctx->stroke();

		// draw line
		ctx->set_source_rgba(1.0, 1.0, 0.5, 0.2);
		ctx->set_line_width(0.01);
		ctx->move_to(player->position().x, player->position().y);
		ctx->line_to(best_shot.first.x, best_shot.first.y);
		ctx->stroke();
	}
	*/

	// draw blue circles for offense
	{

		// divide up into grids
		const double x1 = -world.field().length() / 2;
		const double x2 = world.field().length() / 2;
		const double y1 = -world.field().width() / 2;
		const double y2 = world.field().width() / 2;

		const double dx = (x2 - x1) / (grid_x + 1);
		const double dy = (y2 - y1) / (grid_y + 1);

		for (int i = 0; i < grid_x; ++i) {
			for (int j = 0; j < grid_y; ++j) {
				const double x = x1 + dx * (i + 1);
				const double y = y1 + dy * (j + 1);
				const Point pos = Point(x, y);

				const double score = Evaluation::offense_score(world, pos);

				/*
				   {
				   std::ostringstream text;
				   text << score << std::endl;
				   LOG_INFO(text.str());
				   }
				 */

				if (score < 0) {
					continue;
				}

				const double radius = score * 0.01;

				ctx->set_source_rgba(0.5, 0.5, 1.0, 0.2);
				ctx->arc(x, y, radius, 0.0, 2 * M_PI);
				ctx->fill();
				ctx->stroke();
			}
		}
	}

	// draw green circles for best offense
	{
		std::array<Point, 2> positions = Evaluation::offense_positions(world);
		ctx->set_source_rgba(0.5, 1.0, 0.5, 0.2);
		ctx->arc(positions[0].x, positions[0].y, 0.3, 0.0, 2 * M_PI);
		ctx->fill();
		ctx->stroke();

		ctx->set_source_rgba(0.5, 1.0, 0.5, 0.2);
		ctx->arc(positions[1].x, positions[1].y, 0.3, 0.0, 2 * M_PI);
		ctx->fill();
		ctx->stroke();
	}

	const Field &field = world.field();
	const Point goal1 = Point(field.length() / 2, field.goal_width() / 2);
	const Point goal2 = Point(field.length() / 2, -field.goal_width() / 2);

	// draw enemy goal?
	ctx->set_line_width(0.02);
	ctx->set_source_rgba(1.0, 0.5, 0.5, 1.0);
	ctx->move_to(goal1.x, goal1.y);
	ctx->line_to(goal2.x, goal2.y);
	ctx->stroke();
}

void AI::HL::STP::draw_defense(const World &world, Cairo::RefPtr<Cairo::Context> ctx) {
	const Field &field = world.field();

	const Point goal1 = Point(-field.length() / 2, field.goal_width() / 2);
	const Point goal2 = Point(-field.length() / 2, -field.goal_width() / 2);

	ctx->set_line_width(0.01);
	ctx->set_source_rgba(0.5, 1.0, 0.5, 0.5);

	ctx->move_to(world.ball().position().x, world.ball().position().y);
	ctx->line_to(goal1.x, goal1.y);
	ctx->stroke();

	ctx->move_to(world.ball().position().x, world.ball().position().y);
	ctx->line_to(goal2.x, goal2.y);
	ctx->stroke();

	// draw own goal?
	ctx->set_line_width(0.02);
	ctx->set_source_rgba(1.0, 0.5, 0.5, 1.0);
	ctx->move_to(goal1.x, goal1.y);
	ctx->line_to(goal2.x, goal2.y);
	ctx->stroke();
}

void AI::HL::STP::draw_velocity(const World &world, Cairo::RefPtr<Cairo::Context> ctx) {
	const FriendlyTeam &friendly = world.friendly_team();
	ctx->set_line_width(1.0);
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr player = friendly.get(i);
		double vel_direction = atan(player->velocity().y / player->velocity().x);
		double vel_mag = std::sqrt(player->velocity().y * player->velocity().y + player->velocity().x * player->velocity().x);
		// std::cout << vel_direction << "  " << vel_mag <<std::endl;
		ctx->set_source_rgba(0.0, 0.0, 0.0, 0.2);
		ctx->arc(player->position().x, player->position().y, vel_mag, vel_direction, vel_direction + 1.0);
		ctx->stroke();
	}
}

