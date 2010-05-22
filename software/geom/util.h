#ifndef GEOM_UTIL_H
#define GEOM_UTIL_H

#include "geom/point.h"

/*
   Misc geometry utility functions.
   Contains code ported from 2009 version of AI.
 */

/**
 * Clips a point to a rectangle boundary.
 */
point clip_point(const point& p, const point& bound1, const point& bound2);

/*
   OLD CODE STARTS HERE
 */

// ported code
// WARNING: does not work with parallel lines
// finds the intersection of 2 non-parallel lines
point line_intersect(const point &a, const point &b, const point &c, const point &d);

// ported code
// WARNING: does not work with parallel lines
// there is a ray that shoots out from origin
// the ray is bounded by direction vectors a and b
// want to block this ray with circle of radius r
// where to position the circle?
point calc_block_ray(const point &a, const point &b, const double& radius);

// ported code
// WARNING: output is SIGNED indicating clockwise/counterclockwise direction
// signed line-point distance
double line_point_dist(const point &p, const point &a, const point &b);

// ported code
// tests if 2 line segments crosses each other
bool seg_crosses_seg(const point &a1, const point &a2, const point &b1, const point &b2);

// ported code
// reflects the ray r incident on origin, with normal n
point reflect(const point&v, const point& n);

// ported code
// a = goal post position
// c = ball position
// g = goalie position
// returns the other ray that is not blocked by goalie
point calcBlockOtherRay(const point& a, const point& c, const point& g);

// ported code
// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// checks if goalie blocks goal post
bool goalieBlocksGoalPost(const point& a, const point& b, const point& c, const point& g);

// ported code
// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// finds a defender position to block the ball
point defender_blocks_goal(const point& a, const point& b, const point& c, const point& g, const double& r);

#endif

