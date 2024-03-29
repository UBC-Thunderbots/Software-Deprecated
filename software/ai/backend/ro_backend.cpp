#include "ai/backend/ro_backend.h"
#include <cstdlib>
#include "ai/backend/backend.h"
#include "ai/backend/vision/backend.h"
#include "ai/backend/vision/team.h"

using namespace AI::BE;

namespace
{
/**
 * \brief Returns the port number to use for SSL-Vision data.
 *
 * \return the port number, as a string
 */
const char *vision_port()
{
    const char *evar = std::getenv("SSL_VISION_PORT");
    if (evar)
    {
        return evar;
    }
    else
    {
        // return "10005"; // 10005 is the legacy port
        return "10006";
    }
}

/**
 * \brief A player that cannot be controlled, only viewed.
 */
class ROPlayer final : public Player
{
   public:
    typedef BoxPtr<ROPlayer> Ptr;
    explicit ROPlayer(unsigned int pattern);
    bool has_ball() const override;
    double get_lps(unsigned int index) const override;
    bool chicker_ready() const override;
    bool autokick_fired() const override;
    void tick(bool, bool);
    const Property<Drive::Primitive> &primitive() const override;
    void send_prim(Drive::LLPrimitive p) override;
};

/**
 * \brief A friendly team in the read-only backend.
 */
class FriendlyTeam final : public AI::BE::Vision::Team<ROPlayer, Player>
{
   public:
    explicit FriendlyTeam(Backend &backend);

   protected:
    void create_member(unsigned int pattern) override;
};

/**
 * \brief An enemy team in the read-only backend.
 */
class EnemyTeam final : public AI::BE::Vision::Team<Robot, Robot>
{
   public:
    explicit EnemyTeam(Backend &backend);

   protected:
    void create_member(unsigned int pattern) override;
};

/**
 * \brief A backend which does not allow driving robots, but only viewing their
 * states on the field.
 */
class ROBackend final : public AI::BE::Vision::Backend<FriendlyTeam, EnemyTeam>
{
   public:
    explicit ROBackend(
        const std::vector<bool> &disable_cameras, int multicast_interface);
    BackendFactory &factory() const override;
    FriendlyTeam &friendly_team() override;
    const FriendlyTeam &friendly_team() const override;
    EnemyTeam &enemy_team() override;
    const EnemyTeam &enemy_team() const override;
    void log_to(AI::Logger &) override;

   private:
    void tick() override;
    FriendlyTeam friendly;
    EnemyTeam enemy;
    AI::BE::Vision::VisionSocket vision_rx;
};

/**
 * \brief A factory for creating \ref ROBackend instances.
 */
class ROBackendFactory final : public BackendFactory
{
   public:
    explicit ROBackendFactory();
    std::unique_ptr<Backend> create_backend(
        const std::vector<bool> &disable_cameras,
        int multicast_interface) const override;
};
}

namespace AI
{
namespace BE
{
namespace RO
{
extern ROBackendFactory ro_backend_factory_instance;
}
}
}

ROBackendFactory AI::BE::RO::ro_backend_factory_instance;

ROPlayer::ROPlayer(unsigned int pattern) : Player(pattern)
{
}

bool ROPlayer::has_ball() const
{
    return false;
}

double ROPlayer::get_lps(unsigned int) const
{
    return 0.0;
}

bool ROPlayer::chicker_ready() const
{
    return false;
}

bool ROPlayer::autokick_fired() const
{
    return false;
}

void ROPlayer::tick(bool, bool)
{
    // Do nothing.
}

const Property<Drive::Primitive> &ROPlayer::primitive() const
{
    static const Property<Drive::Primitive> prim(Drive::Primitive::STOP);
    return prim;
}

void ROPlayer::send_prim(Drive::LLPrimitive p)
{
    // Do nothing.
}

FriendlyTeam::FriendlyTeam(Backend &backend)
    : AI::BE::Vision::Team<ROPlayer, Player>(backend)
{
}

void FriendlyTeam::create_member(unsigned int pattern)
{
    members[pattern].create(pattern);
}

EnemyTeam::EnemyTeam(Backend &backend)
    : AI::BE::Vision::Team<Robot, Robot>(backend)
{
}

void EnemyTeam::create_member(unsigned int pattern)
{
    members[pattern].create(pattern);
}

ROBackend::ROBackend(
    const std::vector<bool> &disable_cameras, int multicast_interface)
    : Backend(disable_cameras, multicast_interface),
      friendly(*this),
      enemy(*this),
      vision_rx(multicast_interface, vision_port())
{
    vision_rx.signal_vision_data.connect(
        sigc::mem_fun(this, &Backend::handle_vision_packet));
}

BackendFactory &ROBackend::factory() const
{
    return AI::BE::RO::ro_backend_factory_instance;
}

FriendlyTeam &ROBackend::friendly_team()
{
    return friendly;
}

const FriendlyTeam &ROBackend::friendly_team() const
{
    return friendly;
}

EnemyTeam &ROBackend::enemy_team()
{
    return enemy;
}

const EnemyTeam &ROBackend::enemy_team() const
{
    return enemy;
}

void ROBackend::log_to(AI::Logger &)
{
}

inline void ROBackend::tick()
{
    // If the field geometry is not yet valid, do nothing.
    if (!field_.valid())
    {
        return;
    }

    // Do pre-AI stuff (locking predictors).
    monotonic_time_ = std::chrono::steady_clock::now();
    ball_.lock_time(monotonic_time_);
    friendly_team().lock_time(monotonic_time_);
    enemy_team().lock_time(monotonic_time_);
    for (std::size_t i = 0; i < friendly_team().size(); ++i)
    {
        friendly_team().get_backend_robot(i)->pre_tick();
    }
    for (std::size_t i = 0; i < enemy_team().size(); ++i)
    {
        enemy_team().get_backend_robot(i)->pre_tick();
    }

    // Run the AI.
    signal_tick().emit();

    // Do post-AI stuff (pushing data to the radios and updating predictors).
    for (std::size_t i = 0; i < friendly_team().size(); ++i)
    {
        // test to see if this fixes halt spamming over radio
        friendly_team().get_backend_robot(i)->tick(false, false);
        // friendly_team().get_backend_robot(i)->tick(
        // playtype() == AI::Common::PlayType::HALT,
        // playtype() == AI::Common::PlayType::STOP);
        friendly_team().get_backend_robot(i)->update_predictor(monotonic_time_);
    }

    // Notify anyone interested in the finish of a tick.
    AI::Timestamp after;
    after = std::chrono::steady_clock::now();
    signal_post_tick().emit(after - monotonic_time_);
}

ROBackendFactory::ROBackendFactory() : BackendFactory(u8"ro")
{
}

std::unique_ptr<Backend> ROBackendFactory::create_backend(
    const std::vector<bool> &disable_cameras, int multicast_interface) const
{
    std::unique_ptr<Backend> be(
        new ROBackend(disable_cameras, multicast_interface));
    return be;
}

BackendFactory &AI::BE::RO::get_factory()
{
    return ro_backend_factory_instance;
}
