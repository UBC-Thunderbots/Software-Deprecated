#include "ai/hl/strategy.h"
#include "ai/hl/util.h"
#include "ai/flags.h"
#include "uicomponents/param.h"

#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
#warning find better home for this variable

	// the closest distance players allowed to the ball
	const double AVOIDANCE_MIN = 0.050 + Robot::MAX_RADIUS;

	// make the players slightly further away from the ball
	const double AVOIDANCE_MARGIN = 0.005;

	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = AVOIDANCE_MIN + AVOIDANCE_MARGIN;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	DoubleParam separation_angle("angle to separate players (degrees)", 5, 0, 20);

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class StopStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;
			void stop();

			/**
			 * Creates a new StopStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			StopStrategy(World &world);
			~StopStrategy();
			void on_play_type_changed();
			bool valid(Point p) const;
	};

	/**
	 * A factory for constructing \ref StopStrategy "StopStrategies".
	 */
	class StopStrategyFactory : public StrategyFactory {
		public:
			StopStrategyFactory();
			~StopStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of StopStrategyFactory.
	 */
	StopStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::STOP,
	};

	StrategyFactory &StopStrategy::factory() const {
		return factory_instance;
	}

	bool StopStrategy::valid(Point p) const {
		// cannot be too far away in x
		if (fabs(p.x) > world.field().length() / 2) {
			return false;
		}
		// cannot be too far away in y
		if (fabs(p.y) > world.field().width() / 2) {
			return false;
		}

		if (AI::HL::Util::point_in_friendly_defense(world, p)) {
			return false;
		}

		// TODO: check if point in enemy defense

		// cannot be too close to ball
		if ((world.ball().position() - p).len() < AVOIDANCE_MIN) {
			return false;
		}

		return true;
	}

	void StopStrategy::stop() {
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			return;
		}

		// draw a circle of radius 50cm from the ball
		Point ball_pos = world.ball().position();

		Point start;

		Point goal_pos = world.field().friendly_goal();
		if ((goal_pos - ball_pos).len() < AVOIDANCE_DIST) {
			// if the goal is inside the circle,
			// then shoot the ray from the enemy goal

			start = Point(ball_pos.x + AVOIDANCE_DIST, ball_pos.y);
		} else {
			// otherwise
			// draw a ray from the center of friendly goal to the ball,
			// and the intersection shall be the start point.

			Point ray = (goal_pos - ball_pos).norm();
			start = ball_pos + ray * AVOIDANCE_DIST;
		}

		// check if we want a defender
		int defenders = 0;
		Point defender_pos;
		if (players.size() > 2) {
			defender_pos = (ball_pos + goal_pos) * 0.5;
			if (valid(defender_pos)) {
				defenders = 1;
			}
		}

		// int offenders = static_cast<int>(players.size()) - 1 - defenders;

		// calculate angle between robots
		const double delta_angle = AVOIDANCE_ANGLE + separation_angle;

		const Point shoot = (start - ball_pos);

		// create intervals from the start point
		// place players with the interval points
		unsigned int flags = AI::Flags::FLAG_AVOID_BALL_STOP;
		// the parity determines left or right
		// we only want one of angle = 0, so start at w = 1
		int w = 1;
		for (std::size_t i = 1 + defenders; i < players.size(); ++i) {
			bool okay = false;
			Point p;
			do {
				double angle = delta_angle * (w / 2) * ((w % 2) ? 1 : -1);
				p = ball_pos + shoot.rotate(angle);
				okay = valid(p);
			} while (!okay);
			players[i]->move(p, (world.ball().position() - players[i]->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
		}

		if (defenders) {
			players[1]->move(defender_pos, (world.ball().position() - players[1]->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
		}

#warning incomplete
		// goalie always touching the goal line,
		// and at a point closest to the ball.
		unsigned int goalie_flags = 0;
		Point goalie_pos = world.field().friendly_goal();
		players[0]->move(goalie_pos, (world.ball().position() - players[0]->position()).orientation(), goalie_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}

	Strategy::Ptr StopStrategy::create(World &world) {
		const Strategy::Ptr p(new StopStrategy(world));
		return p;
	}

	StopStrategy::StopStrategy(World &world) : Strategy(world) {
		world.playtype().signal_changed().connect(sigc::mem_fun(this, &StopStrategy::on_play_type_changed));
	}

	StopStrategy::~StopStrategy() {
	}

	void StopStrategy::on_play_type_changed() {
		if (world.playtype() != PlayType::STOP) {
			resign();
		}
	}

	StopStrategyFactory::StopStrategyFactory() : StrategyFactory("Stop", HANDLED_PLAY_TYPES, sizeof(HANDLED_PLAY_TYPES) / sizeof(*HANDLED_PLAY_TYPES)) {
	}

	StopStrategyFactory::~StopStrategyFactory() {
	}

	Strategy::Ptr StopStrategyFactory::create_strategy(World &world) const {
		return StopStrategy::create(world);
	}
}

