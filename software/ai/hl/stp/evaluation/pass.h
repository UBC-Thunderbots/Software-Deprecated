#ifndef AI_HL_STP_EVALUATION_PASS_H
#define AI_HL_STP_EVALUATION_PASS_H

#include "ai/hl/stp/world.h"
#include "geom/param.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Evaluation
{
/**
 * Can pass?
 */
bool can_pass(World world, Player passer, Player passee);

/**
 * Can pass from p1 to p2.
 * Ignores position of friendly robots.
 */
bool can_pass(World world, const Point p1, const Point p2);

/**
 * Checks if a pass is possible for the pair of enemy
 */
bool enemy_can_pass(World world, const Robot passer, const Robot passee);

/**
 * Checks if passee is facing towards the ball so it can receive.
 */
bool passee_facing_ball(World world, Player passee);

/**
 * Check if passee is facing towards passer so it can receive.
 * Not sure why we need passer instead of ball.
 */
bool passee_facing_passer(Player passer, Player passee);

/**
 * Checks if a passee is suitable.
 */
bool passee_suitable(World world, Player passee);

/**
 * Obtains a player who can be a passee.
 * WARNING:
 * This function has built-in hysterysis.
 * Calls will return the previously chosen player if possible.
 */
Player select_passee(World world);

/**
 * Checks if this direction is valid for shooting
 * for indirect pass.
 */
bool can_shoot_ray(World world, Player player, Angle orientation);

/**
 * Calculates the best shooting angle.
 */
std::pair<bool, Angle> best_shoot_ray(World world, const Player player);

Point calc_fastest_grab_ball_dest_if_baller_shoots(
    World world, const Point player_pos);

/**
 * Computes the best location to grab the ball,
 * minimizing the time required.
 *
 * \param[in] ball_pos ball position
 *
 * \param[in] ball_vel ball velocity
 *
 * \param[in] player_pos player position
 *
 * \param[out] dest the location to chase the ball.
 *
 * \return true if the output is valid.
 */
bool calc_fastest_grab_ball_dest(
    Point ball_pos, Point ball_vel, Point player_pos, Point &dest);

extern IntParam ray_intervals;

extern DegreeParam max_pass_ray_angle;

extern DoubleParam ball_pass_velocity;
}
}
}
}

#endif
