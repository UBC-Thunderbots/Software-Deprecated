#ifndef AI_HL_STP_UI_H
#define AI_HL_STP_UI_H

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include "ai/hl/stp/world.h"

namespace AI
{
namespace HL
{
namespace STP
{
/**
 * Draw yellow circles on robots that can shoot the goal.
 */
void draw_shoot(World world, Cairo::RefPtr<Cairo::Context> ctx);

void draw_enemy_pass(World world, Cairo::RefPtr<Cairo::Context> ctx);

void draw_friendly_pass(World world, Cairo::RefPtr<Cairo::Context> ctx);

/**
 * draw blue circles to indicate good offensive position.
 * draw yellow halo around robots to indicate how well they can shoot the enemy
 * goal.
 */
void draw_offense(World world, Cairo::RefPtr<Cairo::Context> ctx);

/**
 * draw lines from the ball to the sides of our goal post
 * TODO: draw lines from the enemy
 */
void draw_defense(World world, Cairo::RefPtr<Cairo::Context> ctx);

void draw_velocity(World world, Cairo::RefPtr<Cairo::Context> ctx);

void draw_player_status(World world, Cairo::RefPtr<Cairo::Context> ctx);

void draw_baller(World world, Cairo::RefPtr<Cairo::Context> ctx);
}
}
}

#endif
