#pragma once

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/cacheable.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Tactic
{
/**
 * Move Stop
 * Active Tactic
 * Move to correct stopping location for the specified player index.
 *
 * \param[in] player_index goes from 0 to 3 and is used for calculating a
 * robot's relative
 * position around the ball.
 */
Tactic::Ptr move_stop(World world, std::size_t player_index);

class StopLocations final
    : public Cacheable<
          std::vector<Point>, CacheableNonKeyArgs<AI::HL::W::World>,
          CacheableKeyArgs<>>
{
   protected:
    std::vector<Point> compute(World w) override;
};

extern StopLocations stop_locations;
}
}
}
}
