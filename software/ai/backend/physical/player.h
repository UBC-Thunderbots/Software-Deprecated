#ifndef AI_BACKEND_PHYSICAL_PLAYER_H
#define AI_BACKEND_PHYSICAL_PLAYER_H

#include <ctime>
#include <utility>
#include <vector>
#include "ai/backend/player.h"
#include "drive/robot.h"
#include "util/annunciator.h"
#include "util/box_ptr.h"

namespace Drive
{
class Robot;
}

namespace AI
{
namespace BE
{
namespace Physical
{
/**
 * \brief A player is a robot that can be driven.
 */
class Player final : public AI::BE::Player
{
   public:
    /**
     * \brief A pointer to a Player.
     */
    typedef BoxPtr<Player> Ptr;

    /**
     * \brief Constructs a new Player object.
     *
     * \param[in] pattern the index of the vision pattern associated with the
     * player.
     *
     * \param[in] bot the robot being driven.
     */
    explicit Player(unsigned int pattern, Drive::Robot &bot);

    /**
     * \brief Destroys a Player object.
     */
    ~Player();

    /**
     * \brief Drives one tick of time through the RobotController and to the
     * radio.
     *
     * \param[in] halt \c true if the current play type is halt, or \c false if
     * not
     *
     * \param[in] stop \c true if the current play type is stop, or \c false if
     * not
     */
    void tick(bool halt, bool stop);

    bool has_ball() const override;
    bool chicker_ready() const override;
    const Property<Drive::Primitive> &primitive() const override;

    void send_prim(Drive::LLPrimitive p) override;

    bool autokick_fired() const override
    {
        return autokick_fired_;
    }
    unsigned int num_bar_graphs() const override;
    double get_lps(unsigned int index) const override;
    double bar_graph_value(unsigned int) const override;
    Visualizable::Colour bar_graph_colour(unsigned int) const override;

   private:
    Drive::Robot &bot;
    Annunciator::Message robot_dead_message;
    bool autokick_fired_;

    void on_autokick_fired();
};
}
}
}

inline const Property<Drive::Primitive> &AI::BE::Physical::Player::primitive()
    const
{
    return bot.primitive;
}

inline void AI::BE::Physical::Player::send_prim(Drive::LLPrimitive p)
{
    bot.send_prim(p);
}

#endif
