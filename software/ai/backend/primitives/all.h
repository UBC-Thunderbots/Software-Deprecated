#pragma once
#include "ai/backend/primitives/primitive.h"

namespace AI
{
namespace BE
{
namespace Primitives
{
class Stop : public Primitive
{
   public:
    inline Stop(AI::Common::Player player, bool coast)
        : Primitive(player, Drive::Primitive::STOP, 0, 0, 0, 0, coast ? 0 : 1)
    {
    }
};

class Move : public Primitive
{
   public:
    inline Move(AI::Common::Player player, Point dest) : Move(player, dest, 0.0)
    {
    }

    inline Move(AI::Common::Player player, Point dest, Angle ori)
        : Move(player, dest, ori, 0.0)
    {
    }

    inline Move(AI::Common::Player player, Point dest, double end_speed)
        : Primitive(
              player, Drive::Primitive::MOVE, dest.x, dest.y, 0, end_speed, 0)
    {
    }

    inline Move(
        AI::Common::Player player, Point dest, Angle orientation, uint8_t extra)
        : Primitive(
              player, Drive::Primitive::MOVE, dest.x, dest.y,
              orientation.to_radians(), 0.0, extra)
    {
    }

    inline Move(
        AI::Common::Player player, Point dest, Angle orientation,
        double end_speed, uint8_t extra)
        : Primitive(
              player, Drive::Primitive::MOVE, dest.x, dest.y,
              orientation.to_radians(), end_speed, extra)
    {
    }
};

class Dribble : public Primitive
{
   public:
    inline Dribble(
        AI::Common::Player player, Point dest, Angle orientation,
        bool small_kick_allowed)
        : Primitive(
              player, Drive::Primitive::DRIBBLE, dest.x, dest.y,
              orientation.to_radians(), 0, small_kick_allowed ? 1 : 0)
    {
    }
};

class Shoot : public Primitive
{
   public:
    inline Shoot(AI::Common::Player player, Point dest, double power, bool chip)
        : Primitive(
              player, Drive::Primitive::SHOOT, dest.x, dest.y, 0.0, power,
              chip ? 1 : 0)
    {
    }

    inline Shoot(
        AI::Common::Player player, Point dest, Angle ori, double power,
        bool chip)
        : Primitive(
              player, Drive::Primitive::SHOOT, dest.x, dest.y, ori.to_radians(),
              power, chip ? 3 : 2)
    {
    }
};

class Catch : public Primitive
{
   public:
    inline Catch(
        AI::Common::Player player, double velocity, double dribbler_speed, double margin)
        : Primitive(
              player, Drive::Primitive::CATCH, velocity, dribbler_speed, margin, 0, 0)
    {
    }
};

class Pivot : public Primitive
{
   public:
    inline Pivot(
        AI::Common::Player player, Point centre, Angle swing, double radius)
        : Primitive(
              player, Drive::Primitive::PIVOT, centre.x, centre.y,
              swing.to_radians(), radius, 0)
    {
    }
};

class Spin : public Primitive
{
   public:
    inline Spin(AI::Common::Player player, Point dest, Angle speed)
        : Primitive(
              player, Drive::Primitive::SPIN, dest.x, dest.y,
              speed.to_radians(), 0, 0)
    {
    }
};
}
}
}
