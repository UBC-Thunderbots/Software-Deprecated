#ifndef GEOM_POINT_H
#define GEOM_POINT_H

#include <ostream>
#include <cmath>

//
// A point or vector in 2D space.
//
class point {
	public:
		//
		// The X coordinate of the point.
		//
		double x;

		//
		// The Y coordinate of the point.
		//
		double y;

		//
		// Creates a point at the origin.
		//
		point() : x(0.0), y(0.0) {
		}

		//
		// Creates a point at arbitrary coordinates.
		//
		point(double x, double y) : x(x), y(y) {
		}

		//
		// Creates a copy of a point.
		//
		point(const point &p) : x(p.x), y(p.y) {
		}

		//
		// Returns the square of the length of the vector.
		//
		double lensq() const __attribute__((warn_unused_result)) {
			return x * x + y * y;
		}

		//
		// Returns the length of the vector.
		//
		double len() const __attribute__((warn_unused_result)) {
			return std::sqrt(lensq());
		}

		//
		// Returns a unit vector in the same direction as this vector, or the
		// zero vector if this is the zero vector.
		//
		point norm() const __attribute__((warn_unused_result)) {
			double l = len();
			point p(x / l, y / l);
			if (std::fabs(p.lensq() - 1.0) < 1.0e-9) {
				return p;
			} else {
				return point();
			}
		}

		//
		// Returns a rotation of the vector.
		//
		point rotate(double rot) const __attribute__((warn_unused_result)) {
			return point(x * std::cos(rot) - y * std::sin(rot), x * std::sin(rot) + y * std::cos(rot));
		}

		//
		// Takes the dot product of two vectors.
		//
		double dot(const point &other) const __attribute__((warn_unused_result)) {
			return x * other.x + y * other.y;
		}

		//
		// Takes the cross product of two vectors.
		//
		double cross(const point &other) const __attribute__((warn_unused_result)) {
			return x * other.y - y * other.x;
		}

		//
		// Assigns one vector to another.
		//
		point &operator=(const point &q) {
			x = q.x;
			y = q.y;
			return *this;
		}
};

namespace {
	//
	// Adds two points.
	//
	point operator+(const point &p, const point &q) __attribute__((warn_unused_result));
	point operator+(const point &p, const point &q) {
		return point(p.x + q.x, p.y + q.y);
	}

	//
	// Adds an offset to a point.
	//
	point &operator+=(point &p, const point &q) {
		p.x += q.x;
		p.y += q.y;
		return p;
	}

	//
	// Negates a point.
	//
	point operator-(const point &p) __attribute__((warn_unused_result));
	point operator-(const point &p) {
		return point(-p.x, -p.y);
	}

	//
	// Subtracts two points.
	//
	point operator-(const point &p, const point &q) __attribute__((warn_unused_result));
	point operator-(const point &p, const point &q) {
		return point(p.x - q.x, p.y - q.y);
	}

	//
	// Subtracts an offset from a point.
	//
	point &operator-=(point &p, const point &q) {
		p.x -= q.x;
		p.y -= q.y;
		return p;
	}

	//
	// Multiplies a vector by a scalar.
	//
	point operator*(double s, const point &p) __attribute__((warn_unused_result));
	point operator*(double s, const point &p) {
		return point(p.x * s, p.y * s);
	}

	//
	// Multiplies a vector by a scalar.
	//
	point operator*(const point &p, double s) __attribute__((warn_unused_result));
	point operator*(const point &p, double s) {
		return point(p.x * s, p.y * s);
	}

	//
	// Scales a vector.
	//
	point &operator*=(point &p, double s) {
		p.x *= s;
		p.y *= s;
		return p;
	}

	//
	// Divides a vector by a scalar.
	//
	point operator/(const point &p, double s) __attribute__((warn_unused_result));
	point operator/(const point &p, double s) {
		return point(p.x / s, p.y / s);
	}

	//
	// Scales a vector.
	//
	point &operator/=(point &p, double s) {
		p.x /= s;
		p.y /= s;
		return p;
	}

	//
	// Prints a vector.
	//
	std::ostream &operator<<(std::ostream &os, const point &p) {
		os << '(' << p.x << ',' << p.y << ')';
		return os;
	}
}

#endif

