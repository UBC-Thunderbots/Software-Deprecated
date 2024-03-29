#ifndef AI_HL_STP_REGION_H
#define AI_HL_STP_REGION_H

#include <cstddef>
#include <functional>
#include "ai/hl/stp/coordinate.h"
#include "geom/rect.h"

namespace AI
{
namespace HL
{
namespace STP
{
/**
 * Describes a region, either a circle or a rectangle.
 * The region can make use of dynamic coordinates.
 * See STP paper section 5.2.3 (b)
 *
 * NOTE: class needs to be COPYABLE,
 * hence it is not subclassed.
 */
class Region final
{
   public:
    enum class Type
    {
        RECTANGLE,
        CIRCLE,
    };

    /**
     * Returns the type of this region.
     */
    Type type() const
    {
        return type_;
    }

    /**
     * Evaluates the position of the center of this region.
     */
    Point center_position() const;

    /**
     * Evaluates the velocity of the center of this region.
     */
    Point center_velocity() const;

    /**
     * Returns a random sample point in this region.
     */
    Point random_sample() const;

    /**
     * Valid only if this region type is a circle.
     * Returns the associated radius.
     */
    double radius() const;

    /**
     * Valid only if this region type is a rectangle.
     * Evaluates the associated rectangle.
     */
    Rect rectangle() const;

    /**
     * Checks if a point in inside this region.
     */
    bool inside(Point p) const;

    /**
     * Create a circle region.
     */
    static Region circle(Coordinate center, double radius);

    /**
     * Create a rectangle region.
     */
    static Region rectangle(Coordinate p1, Coordinate p2);

    /**
     * Copy constructor.
     */
    Region(const Region &region);

    Region &operator=(const Region &r);

    bool operator==(const Region &other) const;

    std::size_t hash() const;

   protected:
    Type type_;
    Coordinate p1, p2;
    double radius_;

    /**
     * The actual constructor, hidden from public view.
     */
    explicit Region(Type type, Coordinate p1, Coordinate p2, double radius);
};
}
}
}

namespace std
{
template <>
struct hash<AI::HL::STP::Region> final
{
    typedef AI::HL::STP::Region argument_type;
    typedef std::size_t result_type;

    std::size_t operator()(const AI::HL::STP::Region &r) const
    {
        return r.hash();
    }
};
}

#endif
