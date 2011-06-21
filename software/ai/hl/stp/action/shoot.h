#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "geom/rect.h"
#include "util/param.h"
#include "ai/hl/util.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Shoots the ball at the largest open angle of the enemy goal.
				 *
				 * \return true if the robot shoots.
				 */
				bool shoot_goal(const World &world, Player::Ptr player);

				/**
				 * WARNING: should use shoot_region or shoot_pass
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 *
				 * \param[in] pass true if the player is to pass.					
				 *
				 * \return true if the robot shoots.
				 */	
				bool shoot_target(const World &world, Player::Ptr player, const Point target, bool pass) __attribute__ ((deprecated));

				/**
				 * Shoots the ball at the region centred at target with radius.
				 *
				 * If the player does not have the ball, chases after it.
				 *
				 * \param[in] target the location to shoot the ball to.
				 *
				 * \return true if the robot shoots.
				 */
				bool shoot_region(const World &world, Player::Ptr player, const Point target, double radius, double delta = 1e9);

				/**
				 * Directly shoots to a player.
				 *
				 * \return true if the shooter shoots.
				 */
				bool shoot_pass(const World& world, Player::Ptr shooter, Player::CPtr receiver);

				/**
				 * Determines whether or not the robot is facing within threshold degrees of the specified target
				 *
				 */
				bool within_angle_thresh(Player::Ptr player, const Point target, double threshold);

				/**
				 * Testing function designed for internal use & use with shoot_distance_test!!!
				 * \param[in] distance in m to shoot the ball.
				 * \param[in] a special ttesting param
				 * \return double the robot kick speed.
				 */
				double shoot_speed(double distance, double delta = 1e9, double alph=-1);

				/**
				 * Arm the kicker so that it kicks ball exact speed to stop at target (i.e. t= inf)
				 * or alternatively reach target at time delta from the current time ( may be moving )
				 * \return true if the constraints are achievable
				 */
				bool arm(const World &world, Player::Ptr player, const Point target, double delta = 1e10);
			}
		}
	}
}

#endif

