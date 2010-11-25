#include "geom/util.h"
#include "geom/angle.h"
#include <algorithm>
#include <cassert>
#include <iostream>

namespace {
	const double EPS = 1e-9;

	// used for lensq only
	const double EPS2 = EPS * EPS;

	// ported code
	inline int sign(const double n) {
		return n > EPS ? 1 : (n < -EPS ? -1 : 0);
	}
}

std::vector<size_t> dist_matching(const std::vector<Point> &v1, const std::vector<Point> &v2) {
	if (v1.size() > 5) {
#warning TODO: use hungarian matching
		std::cerr << "geom: WARNING, hungarian not used yet" << std::endl;
	}
	std::vector<size_t> order(v2.size());
	for (size_t i = 0; i < v2.size(); ++i) {
		order[i] = i;
	}
	std::vector<size_t> best = order;
	double bestscore = 1e99;
	const size_t N = std::min(v1.size(), v2.size());
	do {
		double score = 0;
		for (size_t i = 0; i < N; ++i) {
			score += (v1[i] - v2[order[i]]).len();
		}
		if (score < bestscore) {
			bestscore = score;
			best = order;
		}
	} while (std::next_permutation(order.begin(), order.end()));
	return best;
}

std::pair<Point, double> angle_sweep_circles(const Point &src, const Point &p1, const Point &p2, const std::vector<Point> &obstacles, const double &radius) {
	// default value to return if nothing is valid
	Point bestshot = (p1 + p2) * 0.5;
	const double offangle = (p1 - src).orientation();
	if (collinear(src, p1, p2)) {
		// std::cerr << "geom: collinear " << src << " " << p1 << " " << p2 << std::endl;
		// std::cerr << (p1 - src) << " " << (p2 - src) << std::endl;
		// std::cerr << (p1 - src).cross(p2 - src) << std::endl;
		return std::make_pair(bestshot, 0);
	}
	std::vector<std::pair<double, int> > events;
	events.push_back(std::make_pair(0, 1)); // p1 becomes angle 0
	events.push_back(std::make_pair(angle_mod((p2 - src).orientation() - offangle), -1));
	for (size_t i = 0; i < obstacles.size(); ++i) {
		Point diff = obstacles[i] - src;
		// warning: temporarily reduced
		if (diff.len() < radius) {
			// std::cerr << "geom: inside" << std::endl;
			return std::make_pair(bestshot, 0);
		}
		const double cent = angle_mod(diff.orientation() - offangle);
		const double span = asin(radius / diff.len());
		const double range1 = cent - span;
		const double range2 = cent + span;
		// temporary fix
		if (range1 < -M_PI || range2 > M_PI) {
			continue;
		}
		events.push_back(std::make_pair(range1, -1));
		events.push_back(std::make_pair(range2, 1));
	}
	// do angle sweep for largest angle
	sort(events.begin(), events.end());
	double best = 0;
	double sum = 0;
	double start = events[0].first;
	int cnt = 0;
	for (size_t i = 0; i + 1 < events.size(); ++i) {
		cnt += events[i].second;
		assert(cnt <= 1);
		if (cnt > 0) {
			sum += events[i + 1].first - events[i].first;
			if (best < sum) {
				best = sum;
				// shoot ray from point p
				// intersect with line p1-p2
				const double mid = start + sum / 2 + offangle;
				const Point ray = Point(cos(mid), sin(mid)) * 10.0;
				const Point inter = line_intersect(src, src + ray, p1, p2);
				bestshot = inter;
			}
		} else {
			sum = 0;
			start = events[i + 1].first;
		}
	}
	return std::make_pair(bestshot, best);
}

std::vector<Point> seg_buffer_boundaries(const Point &a, const Point &b, double buffer, int num_points) {
	if ((a - b).lensq() < EPS) {
		return circle_boundaries(a, buffer, num_points);
	}
	std::vector<Point> ans;

	double line_seg = (a - b).len();
	double semi_circle = M_PI * buffer;
	double total_dist = 2 * line_seg + 2 * semi_circle;
	double total_travelled = 0.0;
	double step_len = total_dist / num_points;
	Point add1(0.0, 0.0);
	Point add2 = buffer*((a - b)).rotate(M_PI / 2).norm();
	Point seg_direction = (b - a).norm();
	bool swapped = false;

	for (int i = 0; i < num_points; i++) {
		Point p = a + add1 + add2;
		ans.push_back(p);
		double travel_left = step_len;

		if (total_travelled < line_seg) {
			double l_travel = std::min(travel_left, line_seg - total_travelled);
			add1 += l_travel * seg_direction;
			travel_left -= l_travel;
			total_travelled += l_travel;
		}

		if (travel_left < EPS) {
			continue;
		}

		if (total_travelled + EPS >= line_seg && total_travelled < line_seg + semi_circle) {
			double l_travel = std::min(travel_left, line_seg + semi_circle - total_travelled);
			double rads = l_travel / buffer;
			add2 = add2.rotate(rads);
			travel_left -= l_travel;
			total_travelled += l_travel;
		}

		if (travel_left < EPS) {
			continue;
		}

		if (total_travelled + EPS >= line_seg + semi_circle && total_travelled < 2 * line_seg + semi_circle) {
			if (!swapped) {
				seg_direction = -seg_direction;
				swapped = true;
			}
			double l_travel = std::min(travel_left, 2 * line_seg + semi_circle - total_travelled);
			add1 += l_travel * seg_direction;
			travel_left -= l_travel;
			total_travelled += l_travel;
		}

		if (travel_left < EPS) {
			continue;
		}

		if (total_travelled + EPS >= 2 * line_seg) {
			double rads = travel_left / buffer;
			add2 = add2.rotate(rads);
			total_travelled += travel_left;
		}
	}
	return ans;
}

std::vector<Point> circle_boundaries(const Point &centre, double radius, int num_points) {
	double rotate_amount = 2 * M_PI / num_points;
	std::vector<Point> ans;
	Point bound(radius, 0.0);
	for (int i = 0; i < num_points; i++) {
		Point temp = centre + bound;
		ans.push_back(temp);
		bound = bound.rotate(rotate_amount);
	}
	return ans;
}

bool collinear(const Point &a, const Point &b, const Point &c) {
	if ((a - b).lensq() < EPS2 || (b - c).lensq() < EPS2 || (a - c).lensq() < EPS2) {
		return true;
	}
	return std::fabs((b - a).cross(c - a)) < EPS;
}

#warning this should accept a Rect rather than two points
Point clip_point(const Point &p, const Point &bound1, const Point &bound2) {
	const double minx = std::min(bound1.x, bound2.x);
	const double miny = std::min(bound1.y, bound2.y);
	const double maxx = std::max(bound1.x, bound2.x);
	const double maxy = std::max(bound1.y, bound2.y);
	Point ret = p;
	if (p.x < minx) {
		ret.x = minx;
	} else if (p.x > maxx) {
		ret.x = maxx;
	}
	if (p.y < miny) {
		ret.y = miny;
	} else if (p.y > maxy) {
		ret.y = maxy;
	}
	return ret;
}

std::vector<Point> line_circle_intersect(const Point &centre, double radius, const Point &segA, const Point &segB) {
	std::vector<Point> ans;

	// take care of 0 length segments too much error here
	if ((segB - segA).lensq() < EPS) {
		return ans;
	}

	double lenseg = (segB - segA).dot(centre - segA) / (segB - segA).len();
	Point C = segA + lenseg * (segB - segA).norm();

	// if C outside circle no intersections
	if ((C - centre).lensq() > radius * radius + EPS) {
		return ans;
	}

	// if C on circle perimeter return the only intersection
	if ((C - centre).lensq() < radius * radius + EPS && (C - centre).lensq() > radius * radius - EPS) {
		ans.push_back(C);
		return ans;
	}
	// first possible intersection
	double lensegb = radius * radius - (C - centre).lensq();
	Point inter = C - lensegb * (segB - segA).norm();

	ans.push_back(C - lensegb * (segB - segA).norm());
	ans.push_back(C + lensegb * (segB - segA).norm());

	return ans;
}

std::vector<Point> line_rect_intersect(Rect &r, const Point &segA, const Point &segB) {
	std::vector<Point> ans;
	for (int i = 0; i < 4; i++) {
		int j = i + 1;
		Point a = r[i];
		Point b = r[j];
		if (seg_crosses_seg(a, b, segA, segB) && unique_line_intersect(a, b, segA, segB)) {
			ans.push_back(line_intersect(a, b, segA, segB));
		}
	}
	return ans;
}


double lineseg_point_dist(const Point &centre, const Point &segA, const Point &segB) {
	// if one of the end-points is extremely close to the centre point
	// then return 0.0
	if ((segB - centre).lensq() < EPS2 || (segA - centre).lensq() < EPS2) {
		return 0.0;
	}

	// take care of 0 length segments
	if ((segB - segA).lensq() < EPS2) {
		return std::min((centre - segB).len(), (centre - segA).len());
	}

	// find point C
	// which is the projection onto the line
	double lenseg = (segB - segA).dot(centre - segA) / (segB - segA).len();
	Point C = segA + lenseg * (segB - segA).norm();

	// check if C is in the line seg range
	double AC = (segA - C).lensq();
	double BC = (segB - C).lensq();
	double AB = (segA - segB).lensq();
	bool in_range = AC <= AB && BC <= AB;

	// if so return C
	if (in_range) {
		if ((centre - C).lensq() < EPS2) {
			return 0.0;
		}
		return (centre - C).len();
	}
	// otherwise return distance to closest end of line-seg
	return std::min((centre - segB).len(), (centre - segA).len());
}

double seg_seg_distance(const Point &a, const Point &b, const Point &c, const Point &d) {
	if (seg_crosses_seg(a, b, c, d)) {
		return 0.0;
	}
	return std::min(std::min(lineseg_point_dist(a, c, d), lineseg_point_dist(b, c, d)), std::min(lineseg_point_dist(c, a, b), lineseg_point_dist(d, a, b)));
}

std::vector<Point> lineseg_circle_intersect(const Point &centre, double radius, const Point &segA, const Point &segB) {
	std::vector<Point> ans;
	std::vector<Point> poss = line_circle_intersect(centre, radius, segA, segB);

	for (unsigned int i = 0; i < poss.size(); i++) {
		bool x_ok = poss[i].x <= std::max(segA.x, segB.x) + EPS && poss[i].x >= std::min(segA.x, segB.x) - EPS;
		bool y_ok = poss[i].y <= std::max(segA.y, segB.y) + EPS && poss[i].y >= std::min(segA.y, segB.y) - EPS;
		if (x_ok && y_ok) {
			ans.push_back(poss[i]);
		}
	}
	return ans;
}

bool unique_line_intersect(const Point &a, const Point &b, const Point &c, const Point &d) {
	return sign((d - c).cross(b - a)) != 0;
}

// ported code
Point line_intersect(const Point &a, const Point &b, const Point &c, const Point &d) {
	assert(sign((d - c).cross(b - a)) != 0);
	return a + (a - c).cross(d - c) / (d - c).cross(b - a) * (b - a);
}

// ported code
double line_point_dist(const Point &p, const Point &a, const Point &b) {
	if ((b - a).lensq() < EPS) {
		if ((b - p).lensq() < EPS || (b - p).lensq() < EPS) {
			return 0.0;
		}
		return std::min((b - p).len(), (a - p).len());
	}
	return (p - a).cross(b - a) / (b - a).len();
}


// ported code
#warning this code looks broken (or so geom/util.h used to claim)
bool seg_crosses_seg(const Point &a1, const Point &a2, const Point &b1, const Point &b2) {
	// handle case where the lines are co-linear
	if (sign((a1 - a2).cross(b1 - b2)) == 0) {
		// find distance of two endpoints on segments furthest away from each other
		double mx_len = std::max(std::max((b1 - a2).len(), (b2 - a2).len()), std::max((b1 - a1).len(), (b1 - a2).len()));
		// if the segments cross then this distance should be less than
		// the sum of the distances of the line segments
		return mx_len < (a1 - a2).len() + (b1 - b2).len() + EPS;
	}

	return sign((a2 - a1).cross(b1 - a1))
	       * sign((a2 - a1).cross(b2 - a1)) <= 0 &&
	       sign((b2 - b1).cross(a1 - b1))
	       * sign((b2 - b1).cross(a2 - b1)) <= 0;
}

#warning use pass-by-reference
bool line_seg_intersect_rectangle(Point seg[2], Point recA[4]) {
	bool intersect = point_in_rectangle(seg[0], recA) || point_in_rectangle(seg[1], recA);
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			intersect = intersect || seg_crosses_seg(seg[0], seg[1], recA[i], recA[j]);
		}
	}
	return intersect;
}

#warning use pass-by-reference
bool point_in_rectangle(const Point &pointA, Point recA[4]) {
	bool x_ok = pointA.x >= std::min(std::min(recA[0].x, recA[1].x), std::min(recA[2].x, recA[3].x));
	x_ok = x_ok && pointA.x <= std::max(std::max(recA[0].x, recA[1].x), std::max(recA[2].x, recA[3].x));
	bool y_ok = pointA.y >= std::min(std::min(recA[0].y, recA[1].y), std::min(recA[2].y, recA[3].y));
	y_ok = y_ok && pointA.y <= std::max(std::max(recA[0].y, recA[1].y), std::max(recA[2].y, recA[3].y));

	// was this missing here??
	return x_ok && y_ok;
}

Point reflect(const Point &v, const Point &n) {
	if (n.len() < EPS) {
		std::cerr << "geom: reflect: zero length" << std::endl;
		return v;
	}
	Point normal = n.norm();
	return 2 * v.dot(normal) * normal - v;
}

Point reflect(const Point &a, const Point &b, const Point &p) {
	// Make a as origin.
	// Rotate by 90 degrees, does not matter which direction?
	Point n = (b - a).rotate(M_PI / 2.0);
	return a + reflect(p - a, n);
}

// ported code
Point calc_block_cone(const Point &a, const Point &b, const double &radius) {
	if (a.len() < EPS || b.len() < EPS) {
		std::cerr << "geom: block cone zero vectors" << std::endl;
	}
	// unit vector and bisector
	Point au = a / a.len();
	Point c = au + b / b.len();
	// use similar triangle
	return c * (radius / std::fabs(au.cross(c)));
}

Point calc_block_cone(const Point &a, const Point &b, const Point &p, const double &radius) {
	/*
	   Point R = p + calc_block_cone(a - p, b - p, radius);
	   #warning: THIS MAGIC THRESHOLD should be fixed after competition
	   #warning TODO: Fix this magic number
	   const double MIN_X = std::min(-2.5, (p.x + 3.025) / 2.0 - 3.025);
	   if (R.x < MIN_X){
	   R = (R - p) * ((MIN_X - p.x) / (R.x - p.x)) + p;
	   }
	   return R;
	 */
	return p + calc_block_cone(a - p, b - p, radius);
}

// ported code
#warning Doxygenize this in geom/util.h
#warning what is the "goal post side" (parameter a)?
Point calc_block_other_ray(const Point &a, const Point &c, const Point &g) {
	return reflect(a - c, g - c);
}

// ported code
#warning Doxygenize this in geom/util.h; also, the comments there are unclear (what does it actually do?)
bool goalie_block_goal_post(const Point &a, const Point &b, const Point &c, const Point &g) {
	Point R = reflect(a - c, g - c);
	return R.cross(b - c) < -EPS;
}

// ported code
#warning figure out a, b, and r
Point calc_block_cone_defender(const Point &a, const Point &b, const Point &c, const Point &g, const double &r) {
	Point R = reflect(a - c, g - c);
	// std::cout << (R + c) << std::endl;
	return calc_block_cone(R + c, b, c, r);
}

