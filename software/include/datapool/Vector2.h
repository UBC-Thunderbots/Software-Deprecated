//Generic 2D Vector class

#ifndef TB_VECTOR2_H
#define TB_VECTOR2_H

#include <cmath>

class Vector2 {
public:
	double x;
	double y;

	Vector2() : x(0), y(0) {}
	Vector2(double x, double y) : x(x), y(y) {}
	Vector2(double direction);

	void operator  = (const Vector2&   vector) { x  = vector.x; y  = vector.y; }
	void operator += (const double     scalar) { x += scalar;   y += scalar;   }
	void operator += (const Vector2&   vector) { x += vector.x; y += vector.y; }
	void operator -= (const double     scalar) { x -= scalar;   y -= scalar;   }
	void operator -= (const Vector2&   vector) { x -= vector.x; y -= vector.y; }
	void operator *= (const double     scalar) { x *= scalar;   y *= scalar;   }
	void operator /= (const double     scalar) { x /= scalar;   y /= scalar;   }

	Vector2 operator + (const double     scalar) const { return Vector2(x + scalar,   y + scalar);   }
	Vector2 operator + (const Vector2& 	 vector) const { return Vector2(x + vector.x, y + vector.y); }
	Vector2 operator - (const double     scalar) const { return Vector2(x - scalar,   y - scalar);   }
	Vector2 operator - (const Vector2& 	 vector) const { return Vector2(x - vector.x, y - vector.y); }
	Vector2 operator * (const double     scalar) const { return Vector2(x * scalar,   y * scalar);   }
	Vector2 operator / (const double     scalar) const { return Vector2(x / scalar,   y / scalar);   }

	double length() const                      { return std::sqrt(dot(*this)); }
	double dot   (const Vector2& vector) const { return x * vector.x + y * vector.y; }
	double cross (const Vector2& vector) const { return x * vector.y - y * vector.x; }
	double angle () const;
};

namespace {
	Vector2 operator + (const double scalar, const Vector2 &vector) {
		return vector + scalar;
	}

	Vector2 operator - (const double scalar, const Vector2 &vector) {
		return vector - scalar;
	}

	Vector2 operator * (const double scalar, const Vector2 &vector) {
		return vector * scalar;
	}
}

#endif

