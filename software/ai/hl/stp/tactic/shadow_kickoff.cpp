#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include <algorithm>
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace
{
class ShadowKickoff final : public Tactic
{
   public:
    explicit ShadowKickoff(
        World world, Enemy::Ptr enemy, const Coordinate default_loc)
        : Tactic(world), enemy(enemy), default_loc(default_loc)
    {
    }

   private:
    const Enemy::Ptr enemy;
    const Coordinate default_loc;
    Player select(const std::set<Player>& players) const override;
    void execute(caller_t& ca) override;
    Glib::ustring description() const override
    {
        return u8"shadow kickoff";
    }
};


Player ShadowKickoff::select(const std::set<Player>& players) const
{
    Point location_eval;
    if (enemy->evaluate())
    {
        location_eval = enemy->evaluate().position();
    }
    else
    {
        location_eval = default_loc.position();
    }
    return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(location_eval));
}

void ShadowKickoff::execute(caller_t& ca)
{
    while (true)
    {
        Point targetPosition;
        if (enemy->evaluate())
        {
            // calculate position to block the side enemies from shooting
            // Point block_position =
            // line_intersect(enemy->evaluate().position(),
            // world.field().friendly_goal(), Point(-0.2, 2), Point(-0.2, -2));
            Point enemy_pos = enemy->evaluate().position();
            if (std::fabs(enemy_pos.y) < world.field().centre_circle_radius() + 4 * Robot::MAX_RADIUS)
            {
                targetPosition = Point(
                    -world.field().centre_circle_radius() - 2 * Robot::MAX_RADIUS,
                    enemy_pos.y);
            }
            else
            {
                targetPosition = Point(-2 * Robot::MAX_RADIUS, enemy_pos.y);
            }
        }
        else
        {
            // orient the bot so it faces the center of the field
            targetPosition = default_loc.position();
        }
        Angle orientation = targetPosition.orientation() - Angle::of_degrees(180);
        Action::move(ca, world, player(), targetPosition, orientation);
        yield(ca);
    }
}

class ShadowBall final : public Tactic
{
   public:
    explicit ShadowBall(World world) : Tactic(world)
    {
    }

   private:
    Player select(const std::set<Player>& players) const override;
    void execute(caller_t& ca) override;
    Glib::ustring description() const override
    {
        return u8"shadow ball";
    }
};

Player ShadowBall::select(const std::set<Player>& players) const
{
    return *std::min_element(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(
            Point(world.ball().position().x, -world.ball().position().y)));
}

void ShadowBall::execute(caller_t& ca)
{
    while (true)
    {
        if (world.ball().position().y > 0)
        {
            Action::move(
                ca, world, player(),
                Point(
                    world.ball().position().x,
                    -world.ball().position().y + 2 * Robot::MAX_RADIUS));
        }
        else
        {
            Action::move(
                ca, world, player(),
                Point(
                    world.ball().position().x,
                    -world.ball().position().y - 2 * Robot::MAX_RADIUS));
        }
        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_kickoff(
    World world, Enemy::Ptr enemy, const Coordinate default_loc)
{
    Tactic::Ptr p(new ShadowKickoff(world, enemy, default_loc));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_ball(World world)
{
    Tactic::Ptr p(new ShadowBall(world));
    return p;
}
