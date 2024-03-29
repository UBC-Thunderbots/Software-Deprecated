#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace
{
DoubleParam friendly_pass_width(
    u8"Friendly pass checking width (robot radius)", u8"AI/HL/STP/Pass", 1, 0,
    9);

DoubleParam enemy_pass_width(
    u8"Enemy pass checking width (robot radius)", u8"AI/HL/STP/Pass", 1, 0, 9);

bool can_pass_check(
    const Point p1, const Point p2, const std::vector<Point> &obstacles,
    double tol)
{
    // auto allowance = AI::HL::Util::calc_best_shot_target(passer.position(),
    // obstacles, passee.position(), 1).second;
    // return allowance > degrees2radians(enemy_shoot_accuracy);

    // OLD method is TRIED and TESTED
    return AI::HL::Util::path_check(p1, p2, obstacles, Robot::MAX_RADIUS * tol);
}

bool ray_on_friendly_defense(World world, const Point a, const Point b)
{
    if ((b - a).x > 0)
    {
        return false;
    }
    auto inter = line_circle_intersect(
        world.field().friendly_goal(), goal_avoid_radius, a, b);
    return inter.size() > 0;
}

#warning TOOD: refactor
bool ray_on_friendly_goal(World world, const Point c, const Point d)
{
    if ((d - c).x > 0)
    {
        return false;
    }

    Point a = world.field().friendly_goal() - Point(0, -10);
    Point b = world.field().friendly_goal() + Point(0, 10);

    if (unique_line_intersect(a, b, c, d))
    {
        Point inter = line_intersect(a, b, c, d);
        return inter.y <= std::max(a.y, b.y) && inter.y >= std::min(a.y, b.y);
    }
    return false;
}

DoubleParam pass_ray_threat_mult(
    u8"Ray pass threat multiplier", u8"AI/HL/STP/PassRay", 2, 1, 99);

BoolParam pass_ray_use_calc_fastest(
    u8"Ray pass use calc fastest", u8"AI/HL/STP/PassRay", true);
}

DoubleParam Evaluation::ball_pass_velocity(
    u8"Average ball pass velocity (HACK)", u8"AI/HL/STP/Pass", 2.0, 0, 99);

DegreeParam Evaluation::max_pass_ray_angle(
    u8"Max ray shoot rotation (degrees)", u8"AI/HL/STP/PassRay", 75, 0, 180);

IntParam Evaluation::ray_intervals(
    u8"Ray # of intervals", u8"AI/HL/STP/PassRay", 30, 0, 80);

bool Evaluation::can_shoot_ray(World world, Player player, Angle orientation)
{
    const Point p1 = player.position();
    const Point p2 = p1 + 10 * Point::of_angle(orientation);

    Angle diff = player.orientation().angle_diff(orientation);
    if (diff > max_pass_ray_angle)
    {
        return false;
    }

    // check if the ray heads towards our net
    if (ray_on_friendly_goal(world, p1, p2))
    {
        return false;
    }

    if (ray_on_friendly_defense(world, p1, p2))
    {
        return false;
    }

    double closest_enemy    = 1e99;
    double closest_friendly = 1e99;

    Point ball_vel = Point::of_angle(orientation) * ball_pass_velocity;

    for (const Player fptr : world.friendly_team())
    {
        if (fptr == player)
        {
            continue;
        }

        double dist;

        if (pass_ray_use_calc_fastest)
        {
            Point dest;
            Evaluation::calc_fastest_grab_ball_dest(
                world.ball().position(), ball_vel, fptr.position(), dest);
            dist = (dest - fptr.position()).len();
        }
        else
        {
            dist = Geom::dist(Geom::Seg(p1, p2), fptr.position());
        }

        closest_friendly = std::min(closest_friendly, dist);
    }

    for (const Robot robot : world.enemy_team())
    {
        double dist;
        if (pass_ray_use_calc_fastest)
        {
            Point dest;
            Evaluation::calc_fastest_grab_ball_dest(
                world.ball().position(), ball_vel, robot.position(), dest);
            dist = (dest - robot.position()).len();
        }
        else
        {
            dist = Geom::dist(Geom::Seg(p1, p2), robot.position());
        }

        closest_enemy = std::min(closest_enemy, dist);
    }

    return closest_friendly * pass_ray_threat_mult <= closest_enemy;
}

std::pair<bool, Angle> Evaluation::best_shoot_ray(
    World world, const Player player)
{
    if (!Evaluation::possess_ball(world, player))
    {
        return std::make_pair(false, Angle::zero());
    }

    Angle best_diff  = Angle::of_radians(1e99);
    Angle best_angle = Angle::zero();

    // draw rays for ray shooting

    const Angle angle_span = 2 * max_pass_ray_angle;
    const Angle angle_step = angle_span / Evaluation::ray_intervals;
    const Angle angle_min  = player.orientation() - angle_span / 2;

    for (int i = 0; i < Evaluation::ray_intervals; ++i)
    {
        const Angle angle = angle_min + angle_step * i;

        // const Point p1 = player.position();
        // const Point p2 = p1 + 3 * Point::of_angle(angle);

        Angle diff = player.orientation().angle_diff(angle);

        if (diff > best_diff)
        {
            continue;
        }

        // ok
        if (!Evaluation::can_shoot_ray(world, player, angle))
        {
            continue;
        }

        if (diff < best_diff)
        {
            best_diff  = diff;
            best_angle = angle;
        }
    }

    // cant find good angle
    if (best_diff > Angle::of_radians(1e50))
    {
        return std::make_pair(false, Angle::zero());
    }

    return std::make_pair(true, best_angle);
}

bool Evaluation::enemy_can_pass(
    World world, const Robot passer, const Robot passee)
{
    std::vector<Point> obstacles;
    for (const Player i : world.friendly_team())
    {
        obstacles.push_back(i.position());
    }

    return can_pass_check(
        passer.position(), passee.position(), obstacles, enemy_pass_width);
}

bool Evaluation::can_pass(World world, Player passer, Player passee)
{
    std::vector<Point> obstacles;
    for (const Robot i : world.enemy_team())
    {
        obstacles.push_back(i.position());
    }
    for (const Player i : world.friendly_team())
    {
        if (i == passer)
        {
            continue;
        }
        if (i == passee)
        {
            continue;
        }
        obstacles.push_back(i.position());
    }

    return can_pass_check(
        passer.position(), passee.position(), obstacles, friendly_pass_width);
}

bool Evaluation::can_pass(World world, const Point p1, const Point p2)
{
    std::vector<Point> obstacles;
    for (const Robot i : world.enemy_team())
    {
        obstacles.push_back(i.position());
    }

    return can_pass_check(p1, p2, obstacles, friendly_pass_width);
}

bool Evaluation::passee_facing_ball(World world, Player passee)
{
    return player_within_angle_thresh(
        passee, world.ball().position(), passee_angle_threshold);
}

bool Evaluation::passee_facing_passer(Player passer, Player passee)
{
    return player_within_angle_thresh(
        passee, passer.position(), passee_angle_threshold);
}

bool Evaluation::passee_suitable(World world, Player passee)
{
    if (!passee)
    {
        LOG_ERROR(u8"Passee is null");
        return false;
    }

    // can't pass backwards
    if (passee.position().x < world.ball().position().x)
    {
        return false;
    }

    // must be at least some distance
    if ((passee.position() - world.ball().position()).len() < min_pass_dist)
    {
        return false;
    }

    // must be able to pass
    if (!Evaluation::can_pass(
            world, world.ball().position(), passee.position()))
    {
        return false;
    }

    /*
       if (!Evaluation::passee_facing_ball(world, passee)) {
       return false;
       }
     */

    return true;
}

namespace
{
// hysterysis for select_passee
Player previous_passee;
}

Player Evaluation::select_passee(World world)
{
    std::vector<Player> candidates;
    for (const Player i : world.friendly_team())
    {
        if (possess_ball(world, i))
        {
            continue;
        }
        if (!passee_suitable(world, i))
        {
            continue;
        }

        if (i == previous_passee)
        {
            return i;
        }

        candidates.push_back(i);
    }
    if (candidates.empty())
    {
        return Player();
    }
    random_shuffle(candidates.begin(), candidates.end());
    previous_passee = candidates.front();
    return candidates.front();
}

bool Evaluation::calc_fastest_grab_ball_dest(
    Point ball_pos, Point ball_vel, Point player_pos, Point &dest)
{
    const double ux = ball_vel.len();  // velocity of ball

    const double v = 1.5;

    const Point p1 = ball_pos;

    const Point p2 = player_pos;
    const Point u  = ball_vel.norm();

    const double x = (p2 - p1).dot(u);
    const double y = std::fabs((p2 - p1).cross(u));

    double a = 1 + (y * y) / (x * x);
    double b = (2 * y * y * ux) / (x * x);
    double c = (y * y * ux * ux) / (x * x) - v;

    double vx1 = (-b + std::sqrt(b * b - (4 * a * c))) / (2 * a);
    double vx2 = (-b - std::sqrt(b * b - (4 * a * c))) / (2 * a);

    double t1 = x / (vx1 + ux);
    double t2 = x / (vx2 + ux);

    double t = std::min(t1, t2);
    if (t < 0)
    {
        t = std::max(t1, t2);
    }

    dest = ball_pos;

    if (std::isnan(t) || std::isinf(t) || t < 0)
    {
        return false;
    }

    dest = p1 + ball_vel * 2 * t;

    return true;
}

Point Evaluation::calc_fastest_grab_ball_dest_if_baller_shoots(
    World world, const Point player_pos)
{
    Player baller = Evaluation::calc_friendly_baller();
    if (!baller)
    {
        return world.ball().position();
    }

    Point ball_vel = ball_pass_velocity * Point::of_angle(baller.orientation());
    Point dest;
    Evaluation::calc_fastest_grab_ball_dest(
        world.ball().position(), ball_vel, player_pos, dest);
    return dest;
}
