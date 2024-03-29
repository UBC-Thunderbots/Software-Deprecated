#include "drive/null/robot.h"
#include "drive/null/dongle.h"

Drive::Null::Robot::Robot(Drive::Null::Dongle &dongle)
    : Drive::Robot(0, 1.0, 8.0, 3.0, MAX_DRIBBLER_RPM), the_dongle(dongle)
{
}

Drive::Dongle &Drive::Null::Robot::dongle()
{
    return the_dongle;
}

const Drive::Dongle &Drive::Null::Robot::dongle() const
{
    return the_dongle;
}

void Drive::Null::Robot::set_charger_state(ChargerState)
{
}

void Drive::Null::Robot::send_prim(Drive::LLPrimitive p)
{
}

void Drive::Null::Robot::move_slow(bool)
{
}

void Drive::Null::Robot::direct_wheels(const int (&)[4])
{
}

void Drive::Null::Robot::direct_velocity(Point, Angle)
{
}

void Drive::Null::Robot::direct_dribbler(unsigned int)
{
}

void Drive::Null::Robot::direct_chicker(double, bool)
{
}

void Drive::Null::Robot::direct_chicker_auto(double, bool)
{
}
