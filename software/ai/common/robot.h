#ifndef AI_COMMON_OBJECTS_ROBOT_H
#define AI_COMMON_OBJECTS_ROBOT_H

#include <functional>
#include "ai/backend/robot.h"
#include "ai/common/player.h"

namespace AI
{
namespace Common
{
class Robot;
}
}

namespace std
{
/**
 * \brief Provides a total ordering of Robot objects so they can be stored
 * in STL ordered containers.
 */
template <>
struct less<AI::Common::Robot> final
{
   public:
    /**
     * \brief Compares two objects.
     *
     * \param[in] x the first object
     *
     * \param[in] y the second object
     *
     * \retval true \p x should precede \p y
     * \retval false \p x is equal to or should succeed \p y
     */
    bool operator()(
        const AI::Common::Robot &x, const AI::Common::Robot &y) const;

   private:
    std::less<BoxPtr<AI::BE::Robot>> cmp;
};
}

namespace AI
{
namespace Common
{
/**
 * \brief The common functions available on a robot in all layers.
 */
class Robot
{
   public:
    /**
     * \brief The largest possible radius of a robot, in metres.
     */
    static constexpr double MAX_RADIUS = 0.09;

    /**
     * \brief Constructs a nonexistent Robot.
     */
    explicit Robot();

    /**
     * \brief Constructs a new Robot.
     *
     * \param[in] impl the backend implementation
     */
    explicit Robot(AI::BE::Robot::Ptr impl);

    /**
     * \brief Copies a Robot.
     *
     * \param[in] copyref the object to copy
     */
    Robot(const Robot &copyref);

    /**
     * \brief Checks whether two robots are equal.
     *
     * \param[in] other the robot to compare to
     *
     * \retval true the robots are equal
     * \retval false the robots differ
     */
    bool operator==(const Robot &other) const;

    /**
     * \brief Checks whether two robots are equal.
     *
     * \param[in] other the robot to compare to
     *
     * \retval true the robots differ
     * \retval the robots are equal
     */
    bool operator!=(const Robot &other) const;

    /**
     * \brief Checks whether the robot exists.
     *
     * \retval true the object refers to an existing robot
     * \retval false the object does not refer to an existing robot
     */
    explicit operator bool() const;

    /**
     * \brief Returns the index of the robot.
     *
     * \return the index of the robot's lid pattern
     */
    unsigned int pattern() const;

    /**
     * \brief Replaces the position of the robot.
     *
     * \return a boolean to indicate successful replacement
     */
    virtual bool replace(
        double x, double y, double dir, int id, bool is_yellow);


    /**
     * \brief Gets the predicted current position of the object.
     *
     * \return the predicted position
     */
    Point position() const __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted future position of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the predicted position
     */
    Point position(double delta) const __attribute__((warn_unused_result));

    /**
     * \brief Gets the standard deviation of the predicted position
     * of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the standard deviation of the predicted position
     */
    Point position_stdev(double delta = 0.0) const
        __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted current velocity of the object.
     *
     * \return the predicted velocity
     */
    Point velocity() const __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted future velocity of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the predicted velocity
     */
    Point velocity(double delta) const __attribute__((warn_unused_result));

    /**
     * \brief Gets the standard deviation of the predicted velocity
     * of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the standard deviation of the predicted velocity
     */
    Point velocity_stdev(double delta = 0.0) const
        __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted current orientation of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the predicted orientation
     */
    Angle orientation() const __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted future orientation of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the predicted orientation
     */
    Angle orientation(double delta) const __attribute__((warn_unused_result));

    /**
     * \brief Gets the standard deviation of the predicted
     * orientation of the object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the standard deviation of the predicted orientation
     */
    Angle orientation_stdev(double delta = 0.0) const
        __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted current angular velocity of the
     * object.
     *
     * \return the predicted angular velocity
     */
    Angle avelocity() const __attribute__((warn_unused_result));

    /**
     * \brief Gets the predicted future angular velocity of the
     * object.
     *
     * \param[in] delta the number of seconds forward or backward
     * to predict, relative to the current time
     *
     * \return the predicted angular velocity
     */
    Angle avelocity(double delta) const __attribute__((warn_unused_result));

    /**
     * \brief Gets the standard deviation of the predicted angular velocity of
     * the object.
     *
     * \param[in] delta the number of seconds forward or backward to predict,
     * relative to the current time
     *
     * \return the standard deviation of the predicted angular velocity
     */
    Angle avelocity_stdev(double delta = 0.0) const
        __attribute__((warn_unused_result));

    AI::BE::Robot::Ptr impl;

    friend struct std::less<Robot>;
};
}
}

inline bool std::less<AI::Common::Robot>::operator()(
    const AI::Common::Robot &x, const AI::Common::Robot &y) const
{
    return cmp(x.impl, y.impl);
}

inline AI::Common::Robot::Robot() = default;

inline AI::Common::Robot::Robot(AI::BE::Robot::Ptr impl) : impl(impl)
{
}

inline AI::Common::Robot::Robot(const Robot &) = default;

inline bool AI::Common::Robot::operator==(const Robot &other) const
{
    return impl == other.impl;
}

inline bool AI::Common::Robot::operator!=(const Robot &other) const
{
    return !(*this == other);
}

inline AI::Common::Robot::operator bool() const
{
    return !!impl;
}

inline unsigned int AI::Common::Robot::pattern() const
{
    return impl->pattern();
}


inline bool AI::Common::Robot::replace(
    double x, double y, double dir, int id, bool is_yellow)
{
    return true;
}

inline Point AI::Common::Robot::position() const
{
    return impl->position();
}

inline Point AI::Common::Robot::position(double delta) const
{
    return impl->position(delta);
}

inline Point AI::Common::Robot::position_stdev(double delta) const
{
    return impl->position_stdev(delta);
}

inline Point AI::Common::Robot::velocity() const
{
    return impl->velocity();
}

inline Point AI::Common::Robot::velocity(double delta) const
{
    return impl->velocity(delta);
}

inline Point AI::Common::Robot::velocity_stdev(double delta) const
{
    return impl->velocity_stdev(delta);
}

inline Angle AI::Common::Robot::orientation() const
{
    return impl->orientation();
}

inline Angle AI::Common::Robot::orientation(double delta) const
{
    return impl->orientation(delta);
}

inline Angle AI::Common::Robot::orientation_stdev(double delta) const
{
    return impl->orientation_stdev(delta);
}

inline Angle AI::Common::Robot::avelocity() const
{
    return impl->avelocity();
}

inline Angle AI::Common::Robot::avelocity(double delta) const
{
    return impl->avelocity(delta);
}

inline Angle AI::Common::Robot::avelocity_stdev(double delta) const
{
    return impl->avelocity_stdev(delta);
}

#endif
