#include "ai/hl/stp/param.h"

DoubleParam AI::HL::STP::goal_avoid_radius(
    u8"Avoid goal radius when passing (m)", u8"AI/HL/STP/Pass", 0.9, 0, 10);

RadianParam AI::HL::STP::shoot_accuracy(
    u8"Angle threshold that defines shoot accuracy, bigger is more accurate "
    u8"(radians)",
    u8"AI/HL/STP/Shoot", 0.0, -180.0, 180.0);

DoubleParam AI::HL::STP::shoot_width(
    u8"Shoot accuracy (for various purposes)", u8"AI/HL/STP/Shoot", 5, 0.0,
    180.0);

DoubleParam AI::HL::STP::min_pass_dist(
    u8"Minimum distance for pass play", u8"AI/HL/STP/Pass", 0.5, 0.0, 5.0);

DegreeParam AI::HL::STP::min_shoot_region(
    u8"minimum region available for baller_can_shoot to be true (degrees)",
    u8"AI/HL/STP/param", 0.1 / M_PI * 180.0, 0, 180);

DoubleParam AI::HL::STP::Action::alpha(
    u8"Decay constant for the ball velocity", u8"AI/HL/STP/Action/shoot", 0.1,
    0.0, 1.0);

DegreeParam AI::HL::STP::Action::passer_angle_threshold(
    u8"Angle threshold that defines passing accuracy, smaller is more accurate "
    u8"(degrees)",
    u8"AI/HL/STP/Action/shoot", 5, 0.0, 90.0);

DegreeParam AI::HL::STP::passee_angle_threshold(
    u8"Angle threshold that the passee must be with respect to passer when "
    u8"shot, smaller is more accurate (degrees)",
    u8"AI/HL/STP/Action/shoot", 80.0, 0.0, 90.0);

DoubleParam AI::HL::STP::Action::pass_speed(
    u8"kicking speed for making a pass", u8"AI/HL/STP/Pass", 4.75, 1.0, 10.0);

DoubleParam AI::HL::STP::Action::target_region_param(
    u8" the buffer (meters) in which passee must be with repect to target "
    u8"region before valid ",
    u8"AI/HL/STP/Tactic/pass", 0.0, 0.0, 5.0);

BoolParam AI::HL::STP::Tactic::random_penalty_goalie(
    u8"Whether the penalty goalie should choose random points",
    u8"AI/HL/STP/Tactic/penalty_goalie", false);

DegreeParam AI::HL::STP::Tactic::separation_angle(
    u8"stop: angle to separate players (degrees)", u8"AI/HL/STP/Stop", 15, 0,
    90);
