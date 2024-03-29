#include "ai/common/field.h"

using namespace AI::Common;

Point Field::friendly_goal() const
{
    return Point(-length() * 0.5, 0);
}

Point Field::enemy_goal() const
{
    return Point(length() * 0.5, 0);
}

Point Field::penalty_enemy() const
{
    return Point(length() * 0.5 / 3.025 * (3.025 - 0.750), 0);
}

Point Field::penalty_friendly() const
{
    return Point(-length() * 0.5 / 3.025 * (3.025 - 0.750), 0);
}

Point Field::friendly_corner_pos() const
{
    return Point(friendly_goal().x, width() / 2);
}

Point Field::friendly_corner_neg() const
{
    return Point(friendly_goal().x, -width() / 2);
}

Point Field::enemy_corner_pos() const
{
    return Point(enemy_goal().x, width() / 2);
}

Point Field::enemy_corner_neg() const
{
    return Point(enemy_goal().x, -width() / 2);
}

std::pair<Point, Point> Field::friendly_goal_boundary() const
{
    return std::make_pair(
        Point(-length() * 0.5, -0.5 * goal_width()),
        Point(-length() * 0.5, 0.5 * goal_width()));
}

std::pair<Point, Point> Field::enemy_goal_boundary() const
{
    return std::make_pair(
        Point(length() * 0.5, -0.5 * goal_width()),
        Point(length() * 0.5, 0.5 * goal_width()));
}

double Field::bounds_margin() const
{
    return width() / 20.0;
}

Point Field::friendly_crease_neg_corner() const
{
    return Point(
        -length() / 2 + defense_area_width(), -defense_area_stretch() / 2);
}

Point Field::friendly_crease_pos_corner() const
{
    return Point(
        -length() / 2 + defense_area_width(), defense_area_stretch() / 2);
}

Point Field::friendly_crease_neg_endline() const
{
    return Point(-length() / 2, -defense_area_stretch() / 2);
}
Point Field::friendly_crease_pos_endline() const
{
    return Point(-length() / 2, defense_area_stretch() / 2);
}

Point Field::friendly_goalpost_pos() const
{
    return Point(-length() / 2, defense_area_stretch() / 4);
}

Point Field::friendly_goalpost_neg() const
{
    return Point(-length() / 2, -defense_area_stretch() / 4);
}


Rect Field::friendly_crease() const
{
    Point a = Point(-length() / 2, -defense_area_stretch() / 2);
    Point b = Point(-length() / 2 + defense_area_width(), defense_area_stretch() / 2);
    return Rect(a, b);
}

Rect Field::enemy_crease() const
{
    Point a = Point(length() / 2, -defense_area_stretch() / 2);
    Point b = Point(length() / 2 - defense_area_width(), defense_area_stretch() / 2);
    return Rect(a, b);
}