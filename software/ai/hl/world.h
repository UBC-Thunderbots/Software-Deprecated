#pragma once

#include <functional>
#include "ai/backend/backend.h"
#include "ai/common/world.h"
#include "ai/flags.h"
#include "drive/robot.h"
#include "util/property.h"

namespace AI
{
namespace HL
{
namespace W
{
/**
 * The field
 */
typedef AI::Common::Field Field;

/**
 * The ball
 */
typedef AI::Common::Ball Ball;

/**
 * \brief A robot
 */
class Robot : public AI::Common::Robot
{
   public:
    /**
     * \brief Constructs a nonexistent Robot
     */
    explicit Robot();

    /**
     * \brief Constructs a new Robot
     *
     * \param[in] impl the backend implementation to wrap
     */
    explicit Robot(AI::BE::Robot::Ptr impl);

    /**
     * \brief Constructs a copy of a Robot
     *
     * \param[in] copyref the object to copy
     */
    Robot(const Robot &copyref);

    /**
     * \brief Sets the avoidance distance for this robot
     *
     * If this function is not called, the avoidance distance is reset to medium
     *
     * This function has no effect on friendly robots
     *
     * \param[in] dist the avoidance distance
     */
    void avoid_distance(AI::Flags::AvoidDistance dist);

    /**
     * \brief Returns the avoidance distance for this robot
     *
     * \return the avoidance distance
     */
    AI::Flags::AvoidDistance avoid_distance() const;
};

class World;

/**
 * \brief A player
 */
class Player final : public Robot, public AI::Common::Player
{
   public:
    friend class AI::HL::W::World;

    /**
     * \brief The maximum linear velocity of the robot, in metres per second
     */
    static constexpr double MAX_LINEAR_VELOCITY = 2;

    /**
     * \brief The maximum linear acceleration of the robot, in metres per second
     * squared
     */
    static constexpr double MAX_LINEAR_ACCELERATION = 3.5;

    /**
     * \brief The maximum angular velocity of the robot per second
     */
    static constexpr Angle MAX_ANGULAR_VELOCITY = Angle::of_radians(38);

    /**
     * \brief The maximum angular acceleration of the robot per second squared
     */
    static constexpr Angle MAX_ANGULAR_ACCELERATION = Angle::of_radians(77.7);

    /**
     * \brief Constructs a nonexistent Player
     */
    explicit Player();

    /**
     * \brief Constructs a new Player
     *
     * \param[in] impl the backend implementation to wrap
     */
    explicit Player(AI::BE::Player::Ptr impl);

    /**
     * \brief Constructs a copy of a Player
     *
     * \param[in] copyref the object to copy
     */
    Player(const Player &copyref);

    using AI::Common::Player::operator==;
    using AI::Common::Player::operator!=;
    using AI::Common::Player::operator bool;

    void display_path(const std::vector<Point> &p);

    /**
     * \brief Returns the movement flags for this player
     *
     * \return the flags governing the movement
     */

    AI::Flags::MoveFlags flags() const;

    /**
     * \brief Clear the movement primitives for this player.
     */
    void clear_prims();

    /**
     * \brief Sets the movement flags for this player
     *
     * \param[in] flags the flags governing the movement
     */
    void flags(AI::Flags::MoveFlags flags);

    /**
     * \brief Adds movement flags for this player
     *
     * \param[in] flags the flags governing the movement
     */
    void set_flags(AI::Flags::MoveFlags flags);

    /**
     * \brief Removes movement flags for this player
     *
     * \param[in] flags the flags governing the movement
     */
    void unset_flags(AI::Flags::MoveFlags flags);

    /**
     * \brief Returns the movement priority for this player
     *
     * \return the priority of the movement
     */
    AI::Flags::MovePrio prio() const;

    /**
     * \brief Sets the movement priority for this player
     *
     * \param[in] prio the priority of the movement
     */
    void prio(AI::Flags::MovePrio prio);

    /**
     * \brief Replaces the robot position, direction, team assignment on grSim.
     *
     * \param[in] x-coordinate position to move to, absolute to the
     * robot’s current orientation
     * \param[in] y-coordinate position to move to, absolute to the
     * robot’s current orientation
     * \param[in] direction of robot
     * \param[in] id of the robot
     * \param[in] yellow or blue team assignment of the robot
     */

    bool replace(
        double x, double y, double dir, int id, bool is_yellow) override;

    AI::BE::Player::Ptr get_impl() const;

   private:

};

/**
 * \brief The friendly team
 */
typedef AI::Common::Team<Player, AI::BE::Player> FriendlyTeam;

/**
 * \brief The enemy team
 */
typedef AI::Common::Team<Robot, AI::BE::Robot> EnemyTeam;

/**
 * \brief The world, as seen by a Strategy
 */
class World final
{
   public:
    /**
     * \brief Constructs a new World
     *
     * \param[in] impl the backend implementation
     */
    explicit World(AI::BE::Backend &impl);

    /**
     * \brief Constructs a copy of a World
     *
     * \param[in] copyref the object to copy
     */
    World(const World &copyref);

    /**
     * \brief Compares two world for equality.
     *
     * \param[in] w the world to compare to
     *
     * \return \c true if this world is equal to \p w, or \c false if not
     */
    bool operator==(const World &w) const;

    /**
     * \brief Returns the field
     *
     * \return the field
     */
    const Field &field() const;

    /**
     * \brief Returns the ball
     *
     * \return the ball
     */
    Ball ball() const;

    /**
     * \brief Returns the friendly team
     *
     * \return the friendly team
     */
    FriendlyTeam friendly_team() const;

    /**
     * \brief Returns the enemy team
     *
     * \return the enemy team
     */
    EnemyTeam enemy_team() const;

    /**
     * \brief Returns the current play type
     *
     * \return the current play type
     */
    const Property<AI::Common::PlayType> &playtype() const;

    /**
     * \brief returns the ball placement position
     */
    const Property<Point> &ball_placement_position() const;

    /**
     * \brief Returns the current monotonic time
     *
     * Monotonic time is a way of representing "game time", which always moves
     * forward
     * Monotonic time is consistent within the game world, and may or may not be
     * linked to real time
     * A navigator should \em always use this function to retrieve monotonic
     * time!
     * The AI will not generally have any use for real time
     *
     * \return the current monotonic time
     */
    Timestamp monotonic_time() const;

    /**
    * fix for shitty bots not receiving passes for robocup 2018
    */
#warning update this based on working hardware
        const std::array<unsigned int, 4> working_dribblers = {{6, 3, 0, 1}};

   private:
    AI::BE::Backend &impl;
};
}
}
}

namespace std
{
/**
 * \brief Provides a total ordering of Robot objects so they can be stored in
 * STL ordered containers
 */
template <>
struct less<AI::HL::W::Robot> final
{
   public:
    /**
     * \brief Compares two objects
     *
     * \param[in] x the first objects
     *
     * \param[in] y the second objects
     *
     * \return \c true if \p x should precede \p y in an ordered container, or
     * \c false if not.
     */
    bool operator()(const AI::HL::W::Robot &x, const AI::HL::W::Robot &y) const;

   private:
    std::less<AI::Common::Robot> cmp;
};

/**
 * \brief Provides a total ordering of Player objects so they can be stored in
 * STL ordered containers
 */
template <>
struct less<AI::HL::W::Player> final
{
   public:
    /**
     * \brief Compares two objects
     *
     * \param[in] x the first objects
     *
     * \param[in] y the second objects
     *
     * \return \c true if \p x should precede \p y in an ordered container, or
     * \c false if not.
     */
    bool operator()(
        const AI::HL::W::Player &x, const AI::HL::W::Player &y) const;

   private:
    std::less<AI::Common::Player> cmp;
};
}

inline AI::HL::W::Robot::Robot() = default;

inline AI::HL::W::Robot::Robot(AI::BE::Robot::Ptr impl)
    : AI::Common::Robot(impl)
{
}

inline AI::HL::W::Robot::Robot(const Robot &) = default;

inline AI::Flags::AvoidDistance AI::HL::W::Robot::avoid_distance() const
{
    return impl->avoid_distance();
}

inline void AI::HL::W::Robot::avoid_distance(AI::Flags::AvoidDistance dist)
{
    impl->avoid_distance(dist);
}

inline AI::HL::W::Player::Player() = default;

inline AI::HL::W::Player::Player(AI::BE::Player::Ptr impl)
    : AI::HL::W::Robot(impl), AI::Common::Player(impl)
{
}

inline AI::HL::W::Player::Player(const Player &) = default;

inline AI::Flags::MoveFlags AI::HL::W::Player::flags() const
{
    return AI::Common::Player::impl->flags();
}

inline void AI::HL::W::Player::flags(AI::Flags::MoveFlags flags)
{
    AI::Common::Player::impl->flags(flags);
}

inline void AI::HL::W::Player::set_flags(AI::Flags::MoveFlags flags)
{
    AI::Common::Player::impl->flags(AI::Common::Player::impl->flags() | flags);
}

inline void AI::HL::W::Player::unset_flags(AI::Flags::MoveFlags flags)
{
    AI::Common::Player::impl->flags(AI::Common::Player::impl->flags() & ~flags);
}

inline void AI::HL::W::Player::clear_prims()
{
    AI::Common::Player::impl->clear_prims();
}

inline AI::Flags::MovePrio AI::HL::W::Player::prio() const
{
    return AI::Common::Player::impl->prio();
}

inline void AI::HL::W::Player::prio(AI::Flags::MovePrio prio)
{
    AI::Common::Player::impl->prio(prio);
}

inline AI::BE::Player::Ptr AI::HL::W::Player::get_impl() const
{
    return AI::Common::Player::impl;
}

inline AI::HL::W::World::World(AI::BE::Backend &impl) : impl(impl)
{
}

inline AI::HL::W::World::World(const World &) = default;

inline const AI::HL::W::Field &AI::HL::W::World::field() const
{
    return impl.field();
}

inline AI::HL::W::Ball AI::HL::W::World::ball() const
{
    return AI::Common::Ball(impl.ball());
}

inline AI::HL::W::FriendlyTeam AI::HL::W::World::friendly_team() const
{
    return FriendlyTeam(impl.friendly_team());
}

inline AI::HL::W::EnemyTeam AI::HL::W::World::enemy_team() const
{
    return EnemyTeam(impl.enemy_team());
}

inline AI::Timestamp AI::HL::W::World::monotonic_time() const
{
    return impl.monotonic_time();
}

inline bool AI::HL::W::World::operator==(const World &w) const
{
    return &impl == &w.impl;
}

inline const Property<AI::Common::PlayType> &AI::HL::W::World::playtype() const
{
    return impl.playtype();
}

inline const Property<Point> &AI::HL::W::World::ball_placement_position() const
{
    return impl.ball_placement_position();
}

inline bool std::less<AI::HL::W::Robot>::operator()(
    const AI::HL::W::Robot &x, const AI::HL::W::Robot &y) const
{
    return cmp(x, y);
}

inline bool std::less<AI::HL::W::Player>::operator()(
    const AI::HL::W::Player &x, const AI::HL::W::Player &y) const
{
    return cmp(x, y);
}

inline void AI::HL::W::Player::display_path(const std::vector<Point> &p)
{
    AI::Common::Player::impl->display_path(p);
}

inline bool AI::HL::W::Player::replace(
    double x, double y, double dir, int id, bool is_yellow)
{
    return false;
}
