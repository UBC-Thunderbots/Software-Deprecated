#include "ai/logger.h"
#include <fcntl.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>
#include <google/protobuf/io/coded_stream.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <limits>
#include <locale>
#include <ratio>
#include <sstream>
#include "ai/backend/primitives/primitive.h"
#include "log/shared/enums.h"
#include "log/shared/magic.h"
#include "util/algorithm.h"
#include "util/annunciator.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/param.h"
#include "util/timestep.h"

namespace
{
FileDescriptor create_file()
{
    const std::string &parent_dir = Glib::get_user_data_dir();
    const std::string &tbots_dir =
        Glib::build_filename(parent_dir, "thunderbots");
    mkdir(tbots_dir.c_str(), 0777);
    const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
    mkdir(logs_dir.c_str(), 0777);
    std::time_t tim;
    if (!std::time(&tim))
    {
        throw SystemError("time", errno);
    }
    std::tm tm;
    if (!localtime_r(&tim, &tm))
    {
        throw SystemError("localtime_r", errno);
    }
    std::ostringstream buffer;
    static const char PATTERN[] = "%Y-%m-%d %H:%M:%S";
    std::use_facet<std::time_put<char>>(std::locale())
        .put(
            buffer, buffer, L' ', &tm, PATTERN, PATTERN + std::strlen(PATTERN));
    const std::string &filename = Glib::build_filename(logs_dir, buffer.str());
    return FileDescriptor::create_open(
        filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
}

int32_t encode_micros(double in)
{
    const double min = std::numeric_limits<int32_t>::min();
    const double max = std::numeric_limits<int32_t>::max();
    return static_cast<int32_t>(clamp(in * 1000000.0, min, max));
}

uint32_t encode_micros_unsigned(double in)
{
    return static_cast<uint32_t>(encode_micros(in));
}

void timestamp_to_log(
    const AI::Timestamp &src, const AI::Timestamp &ref,
    Log::MonotonicTimeSpec &dest)
{
    AI::Timediff diff = src - ref;
    int64_t nanos =
        std::chrono::duration_cast<std::chrono::duration<int64_t, std::nano>>(
            diff)
            .count();
    dest.set_seconds(nanos / 1000000000);
    dest.set_nanoseconds(static_cast<int32_t>(nanos % 1000000000));
}

Log::Colour colour_to_protobuf(AI::Common::Colour clr)
{
    switch (clr)
    {
        case AI::Common::Colour::YELLOW:
            return Log::COLOUR_YELLOW;

        case AI::Common::Colour::BLUE:
            return Log::COLOUR_BLUE;
    }
    throw std::invalid_argument("Invalid enumeration element.");
}

AI::Logger *instance = nullptr;
}

namespace Primitives = AI::BE::Primitives;

void ai_logger_signal_handler_thunk(int sig)
{
    instance->signal_handler(sig);
    raise(sig);
}

AI::Logger::Logger(const AI::AIPackage &ai)
    : ai(ai),
      fd(create_file()),
      fos(fd.fd()),
      ended(false),
      sigstack_registration(sigstack, sizeof(sigstack)),
      SIGHUP_registration(
          SIGHUP, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGINT_registration(
          SIGINT, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGQUIT_registration(
          SIGQUIT, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGILL_registration(
          SIGILL, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGTRAP_registration(
          SIGTRAP, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGABRT_registration(
          SIGABRT, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGBUS_registration(
          SIGBUS, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGFPE_registration(
          SIGFPE, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGSEGV_registration(
          SIGSEGV, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGPIPE_registration(
          SIGPIPE, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGTERM_registration(
          SIGTERM, &ai_logger_signal_handler_thunk, SA_RESETHAND),
      SIGSTKFLT_registration(
          SIGSTKFLT, &ai_logger_signal_handler_thunk, SA_RESETHAND)
{
    // Write the magic string.
    {
        google::protobuf::io::CodedOutputStream cos(&fos);
        cos.WriteString(Log::MAGIC);
    }

    // Write the requisite first record, startup_time.
    {
        Log::Record record;
        record.mutable_startup_time()->set_seconds(
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()));
        record.mutable_startup_time()->set_nanoseconds(0);
        write_record(record);
    }

    // Write the requisite second record, config.
    config_record.mutable_config()->set_backend(ai.backend.factory().name());
    if (ai.high_level.get())
    {
        config_record.mutable_config()->set_high_level(
            ai.high_level->factory().name());
    }
    config_record.mutable_config()->set_friendly_colour(
        colour_to_protobuf(ai.backend.friendly_colour()));
    config_record.mutable_config()->set_nominal_ticks_per_second(
        TIMESTEPS_PER_SECOND);
    write_record(config_record);

    // Write the requisite third record, parameters.
    {
        Log::Record record;
        add_params_to_record(record, ParamTreeNode::root());
        write_record(record);
    }

    // Write the requisite fourth record, scores.
    on_score_changed();

    signal_message_logged.connect(
        sigc::mem_fun(this, &AI::Logger::on_message_logged));

    Annunciator::signal_message_activated.connect(
        sigc::mem_fun(this, &AI::Logger::on_annunciator_message_activated));
    Annunciator::signal_message_deactivated.connect(
        sigc::mem_fun(this, &AI::Logger::on_annunciator_message_deactivated));
    Annunciator::signal_message_reactivated.connect(
        sigc::mem_fun(this, &AI::Logger::on_annunciator_message_reactivated));
    Annunciator::signal_message_text_changed.connect(
        sigc::mem_fun(this, &AI::Logger::on_annunciator_message_text_changed));

    attach_param_change_handler(ParamTreeNode::root());

    ai.backend.signal_vision().connect(
        sigc::mem_fun(this, &AI::Logger::on_vision_packet));
    ai.backend.signal_refbox().connect(
        sigc::mem_fun(this, &AI::Logger::on_refbox_packet));
    ai.backend.field().signal_changed.connect(
        sigc::mem_fun(this, &AI::Logger::on_field_changed));
    ai.backend.friendly_colour().signal_changed().connect(
        sigc::mem_fun(this, &AI::Logger::on_friendly_colour_changed));
    ai.backend.friendly_team().score.signal_changed().connect(
        sigc::mem_fun(this, &AI::Logger::on_score_changed));
    ai.backend.enemy_team().score.signal_changed().connect(
        sigc::mem_fun(this, &AI::Logger::on_score_changed));
    ai.backend.signal_post_tick().connect(
        sigc::mem_fun(this, &AI::Logger::on_tick));
    ai.high_level.signal_changed().connect(
        sigc::mem_fun(this, &AI::Logger::on_high_level_changed));
    ai.signal_ai_notes_changed.connect(
        sigc::mem_fun(this, &AI::Logger::on_ai_notes_changed));

    // Field geometry may already be valid and consequently may not ever change;
    // thus, if the geometry is already valid, log it.
    if (ai.backend.field().valid())
    {
        on_field_changed();
    }

    instance = this;
}

AI::Logger::~Logger()
{
    instance = nullptr;
    if (!ended)
    {
        try
        {
            Log::Record record;
            record.mutable_shutdown()->mutable_normal();
            write_record(record);
        }
        catch (...)
        {
            // Swallow; failing to write the shutdown record is not the worst of
            // our problems.
        }
    }
}

void AI::Logger::end_with_exception(const Glib::ustring &msg)
{
    Log::Record record;
    record.mutable_shutdown()->mutable_exception()->set_message(msg);
    write_record(record);
    flush();
    ended = true;
}

void AI::Logger::log_mrf_drive(const void *data, std::size_t length)
{
    Log::Record record;
    record.mutable_mrf()->set_drive_packet(data, length);
    write_record(record);
}

void AI::Logger::log_mrf_message_out(
    unsigned int index, bool reliable, unsigned int id, const void *data,
    std::size_t length)
{
    Log::Record record;
    Log::MRF::OutMessage *msg = record.mutable_mrf()->mutable_out_message();
    msg->set_index(index);
    if (reliable)
    {
        msg->set_id(id);
    }
    msg->set_data(data, length);
    write_record(record);
}

void AI::Logger::log_mrf_message_in(
    unsigned int index, const void *data, std::size_t length, unsigned int lqi,
    unsigned int rssi)
{
    Log::Record record;
    Log::MRF::InMessage *msg = record.mutable_mrf()->mutable_in_message();
    msg->set_index(index);
    msg->set_data(data, length);
    msg->set_lqi(lqi);
    msg->set_rssi(rssi);
    write_record(record);
}

void AI::Logger::log_mrf_mdr(unsigned int id, unsigned int code)
{
    Log::Record record;
    Log::MRF::MDR *mdr = record.mutable_mrf()->mutable_mdr();
    mdr->set_id(id);
    mdr->set_code(code);
    write_record(record);
}

void AI::Logger::write_record(const Log::Record &record)
{
    assert(record.IsInitialized());
    google::protobuf::io::CodedOutputStream cos(&fos);
    cos.WriteVarint32(static_cast<uint32_t>(record.ByteSize()));
    record.SerializeWithCachedSizes(&cos);
    if (cos.HadError())
    {
        throw std::runtime_error("Failed to serialize log record.");
    }
}

void AI::Logger::flush()
{
    fos.Flush();
    fsync(fd.fd());
}

void AI::Logger::add_params_to_record(
    Log::Record &record, const ParamTreeNode *node)
{
    const Param *param = dynamic_cast<const Param *>(node);
    if (param)
    {
        Log::Parameter &p = *record.add_parameters();
        p.set_name(param->name());
        param->encode_value_to_log(p);
    }

    for (std::size_t i = 0; i < node->num_children(); ++i)
    {
        add_params_to_record(record, node->child(i));
    }
}

void AI::Logger::attach_param_change_handler(ParamTreeNode *node)
{
    Param *p = dynamic_cast<Param *>(node);
    if (p)
    {
        p->signal_changed().connect(
            sigc::bind(sigc::mem_fun(this, &AI::Logger::on_param_changed), p));
    }
    for (std::size_t i = 0; i < node->num_children(); ++i)
    {
        attach_param_change_handler(node->child(i));
    }
}

void AI::Logger::signal_handler(int sig)
{
    Log::Record record;
    record.mutable_shutdown()->mutable_signal()->set_signal(
        static_cast<uint32_t>(sig));
    write_record(record);
    flush();
    ended = true;
}

void AI::Logger::on_message_logged(
    Log::DebugMessageLevel level, const Glib::ustring &msg)
{
    Log::Record record;
    record.mutable_debug_message()->set_level(level);
    record.mutable_debug_message()->set_text(msg);
    write_record(record);
}

void AI::Logger::log_annunciator(std::size_t i, bool activated)
{
    const Annunciator::Message &message = *Annunciator::visible()[i];
    Log::Record record;
    record.mutable_annunciator_message()->set_text(message.get_text());
    switch (message.mode)
    {
        case Annunciator::Message::TriggerMode::LEVEL:
            if (activated)
            {
                record.mutable_annunciator_message()->set_action(
                    Log::ANNUNCIATOR_ACTION_ASSERT);
                write_record(record);
            }
            else
            {
                record.mutable_annunciator_message()->set_action(
                    Log::ANNUNCIATOR_ACTION_DEASSERT);
                write_record(record);
            }
            break;

        case Annunciator::Message::TriggerMode::EDGE:
            if (activated)
            {
                record.mutable_annunciator_message()->set_action(
                    Log::ANNUNCIATOR_ACTION_EDGE);
                write_record(record);
            }
            break;
    }
}

void AI::Logger::on_annunciator_message_activated()
{
    log_annunciator(Annunciator::visible().size() - 1, true);
}

void AI::Logger::on_annunciator_message_deactivated(std::size_t i)
{
    log_annunciator(i, false);
}

void AI::Logger::on_annunciator_message_reactivated(std::size_t i)
{
    log_annunciator(i, true);
}

void AI::Logger::on_annunciator_message_text_changed(std::size_t i)
{
    log_annunciator(i, true);
}

void AI::Logger::on_param_changed(const Param *p)
{
    Log::Record record;
    Log::Parameter &param = *record.add_parameters();
    param.set_name(p->name());
    p->encode_value_to_log(param);
    write_record(record);
}

void AI::Logger::on_vision_packet(
    AI::Timestamp ts, const SSL_WrapperPacket &vision_packet)
{
    Log::Record record;
    timestamp_to_log(
        ts, ai.backend.monotonic_start_time(),
        *record.mutable_vision()->mutable_timestamp());
    record.mutable_vision()->mutable_data()->CopyFrom(vision_packet);
    write_record(record);
}

void AI::Logger::on_refbox_packet(AI::Timestamp ts, const SSL_Referee &packet)
{
    Log::Record record;
    timestamp_to_log(
        ts, ai.backend.monotonic_start_time(),
        *record.mutable_refbox()->mutable_timestamp());
    *record.mutable_refbox()->mutable_new_data() = packet;
    write_record(record);
}

void AI::Logger::on_field_changed()
{
    Log::Record record;
    const AI::Common::Field &field =
        static_cast<const AI::HL::W::Field &>(ai.backend.field());
    record.mutable_field()->set_length(encode_micros_unsigned(field.length()));
    record.mutable_field()->set_total_length(
        encode_micros_unsigned(field.total_length()));
    record.mutable_field()->set_width(encode_micros_unsigned(field.width()));
    record.mutable_field()->set_total_width(
        encode_micros_unsigned(field.total_width()));
    record.mutable_field()->set_goal_width(
        encode_micros_unsigned(field.goal_width()));
    record.mutable_field()->set_centre_circle_radius(
        encode_micros_unsigned(field.centre_circle_radius()));
    record.mutable_field()->set_defense_area_radius(
        encode_micros_unsigned(field.defense_area_width()));
    record.mutable_field()->set_defense_area_stretch(
        encode_micros_unsigned(field.defense_area_stretch()));
    write_record(record);
}

void AI::Logger::on_friendly_colour_changed()
{
    config_record.mutable_config()->set_friendly_colour(
        colour_to_protobuf(ai.backend.friendly_colour()));
    write_record(config_record);
}

void AI::Logger::on_high_level_changed()
{
    if (ai.high_level.get())
    {
        config_record.mutable_config()->set_high_level(
            ai.high_level->factory().name());
    }
    else
    {
        config_record.mutable_config()->clear_high_level();
    }
    write_record(config_record);
    Log::Record empty_ai_notes_record;
    empty_ai_notes_record.set_ai_notes(u8"");
    write_record(empty_ai_notes_record);
}

void AI::Logger::on_score_changed()
{
    Log::Record record;
    record.mutable_scores()->set_friendly(ai.backend.friendly_team().score);
    record.mutable_scores()->set_enemy(ai.backend.enemy_team().score);
    write_record(record);
}

void AI::Logger::on_ai_notes_changed(const Glib::ustring &notes)
{
    Log::Record record;
    record.set_ai_notes(notes);
    write_record(record);
}

void AI::Logger::on_tick(AI::Timediff compute_time)
{
    {
        Log::Record record;
        Log::Tick &tick = *record.mutable_tick();
        tick.set_play_type(
            Log::Util::PlayType::to_protobuf(ai.backend.playtype()));
        timestamp_to_log(
            ai.backend.monotonic_time(), ai.backend.monotonic_start_time(),
            *tick.mutable_start_time());
        tick.set_compute_time(
            std::chrono::duration_cast<
                std::chrono::duration<unsigned int, std::nano>>(compute_time)
                .count());
        {
            Log::Tick::Ball &ball = *tick.mutable_ball();
            const AI::BE::Ball &b = ai.backend.ball();
            encode_vec2(b.position(), *ball.mutable_position());
            encode_vec2(b.velocity(), *ball.mutable_velocity());
            encode_vec2(b.position_stdev(0), *ball.mutable_position_stdev());
            encode_vec2(b.velocity_stdev(0), *ball.mutable_velocity_stdev());
        }

        for (std::size_t i = 0; i < ai.backend.friendly_team().size(); ++i)
        {
            Log::Tick::FriendlyRobot &player = *tick.add_friendly_robots();
            AI::BE::Player::Ptr p = ai.backend.friendly_team().get(i);
            player.set_pattern(p->pattern());
            encode_vec3(
                p->position(), p->orientation(), *player.mutable_position());
            encode_vec3(
                p->velocity(), p->avelocity(), *player.mutable_velocity());
            player.set_movement_flags(static_cast<uint64_t>(p->flags()));

            // Deprecated
            player.set_movement_type(Log::MoveType::MOVE_TYPE_NORMAL);
            player.set_movement_priority(
                Log::Util::MovePrio::to_protobuf(p->prio()));

            if (p->has_prim())
            {
                Log::Tick::FriendlyRobot::HLPrimitive &prim =
                    *player.mutable_hl_primitive();
                Primitives::PrimitiveDescriptor desc = p->top_prim()->desc();
                prim.set_primitive(
                    Log::Util::Primitive::to_protobuf(desc.prim));
                prim.set_extra(desc.extra);

                for (int i = 0; i < 4; i++)
                {
                    prim.add_params(desc.params[i]);
                }
            }

            for (Point j : p->display_path())
            {
                Log::Vector2 &path_element = *player.add_display_path();
                encode_vec2(j, path_element);
            }

            for (unsigned int i = 0; i != 4; ++i)
            {
                player.add_lps(p->get_lps(i));
            }

#warning Log some more information related to navigator-output movement primitives!
        }

        for (std::size_t i = 0; i < ai.backend.enemy_team().size(); ++i)
        {
            Log::Tick::EnemyRobot &robot = *tick.add_enemy_robots();
            AI::BE::Robot::Ptr r         = ai.backend.enemy_team().get(i);
            robot.set_pattern(r->pattern());
            encode_vec3(
                r->position(), r->orientation(), *robot.mutable_position());
            encode_vec3(
                r->velocity(), r->avelocity(), *robot.mutable_velocity());
        }

        write_record(record);
    }

    // lps
    /*
    {
            Log::Record record;
            for (std::size_t i = 0; i < ai.backend.friendly_team().size(); ++i)
    {
                    AI::BE::Player::Ptr p = ai.backend.friendly_team().get(i);
                            record.mutable_lps()->set_val1(p->get_lps(1));
                            record.mutable_lps()->set_val2(p->get_lps(2));
                            record.mutable_lps()->set_val3(p->get_lps(3));
                            record.mutable_lps()->set_val4(p->get_lps(4));
                            break;

            }
            write_record(record);
    }
    */
}

void AI::Logger::encode_vec2(Point p, Log::Vector2 &log)
{
    log.set_x(encode_micros(p.x));
    log.set_y(encode_micros(p.y));
}

void AI::Logger::encode_vec3(Point p, Angle a, Log::Vector3 &log)
{
    log.set_x(encode_micros(p.x));
    log.set_y(encode_micros(p.y));
    log.set_t(encode_micros(a.to_radians()));
}
