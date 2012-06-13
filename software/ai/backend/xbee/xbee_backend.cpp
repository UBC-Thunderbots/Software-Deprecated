#include "ai/backend/xbee/xbee_backend.h"
#include "ai/backend/backend.h"
#include "ai/backend/xbee/ball.h"
#include "ai/backend/xbee/field.h"
#include "ai/backend/xbee/player.h"
#include "ai/backend/xbee/refbox.h"
#include "ai/backend/xbee/robot.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/box_array.h"
#include "util/clocksource_timerfd.h"
#include "util/codec.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/sockaddrs.h"
#include "util/timestep.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <cassert>
#include <cstring>
#include <locale>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

DoubleParam XBEE_LOOP_DELAY("Loop Delay", "Backend/XBee", 0.0, -1.0, 1.0);

using namespace AI::BE;

namespace {
	/**
	 * \brief The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
	 */
	const double BALL_FREE_DISTANCE = 0.09;

	/**
	 * \brief The number of vision failures to tolerate before assuming the robot is gone and removing it from the system.
	 *
	 * Note that this should be fairly high because the failure count includes instances of a packet arriving from a camera that cannot see the robot
	 * (this is expected to cause a failure to be counted which will then be zeroed out a moment later as the other camera sends its packet).
	 */
	const unsigned int MAX_VISION_FAILURES = 120;

	class XBeeBackend;

	/**
	 * \brief A generic team.
	 *
	 * \tparam T the type of robot on this team, either Player or Robot.
	 *
	 * \tparam TSuper the type of the superclass of the robot, one of the backend Player or Robot classes.
	 *
	 * \tparam Super the type of the class's superclass.
	 */
	template<typename T, typename TSuper, typename Super> class GenericTeam : public Super {
		public:
			/**
			 * \brief Constructs a new GenericTeam.
			 *
			 * \param[in] backend the backend to which the team is attached.
			 */
			explicit GenericTeam(XBeeBackend &backend);

			/**
			 * \brief Returns the number of existent robots in the team.
			 *
			 * \return the number of existent robots in the team.
			 */
			std::size_t size() const;

			/**
			 * \brief Returns a robot.
			 *
			 * \param[in] i the index of the robot to fetch.
			 *
			 * \return the robot.
			 */
			typename T::Ptr get_xbee_robot(std::size_t i);

			/**
			 * \brief Returns a robot.
			 *
			 * \param[in] i the index of the robot to fetch.
			 *
			 * \return the robot.
			 */
			typename TSuper::Ptr get(std::size_t i);

			/**
			 * \brief Returns a robot.
			 *
			 * \param[in] i the index of the robot to fetch.
			 *
			 * \return the robot.
			 */
			typename TSuper::CPtr get(std::size_t i) const;

			/**
			 * \brief Removes all robots from the team.
			 */
			void clear();

			/**
			 * \brief Updates the robots on the team using data from SSL-Vision.
			 *
			 * \param[in] packets the packets to extract vision data from.
			 *
			 * \param[in] ts the time at which the packet was received.
			 */
			void update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts);

			/**
			 * \brief Locks a time for prediction across all players on the team.
			 *
			 * \param[in] now the time to lock as time zero.
			 */
			void lock_time(const timespec &now);

		protected:
			XBeeBackend &backend;
			BoxArray<T, 16> members;
			std::vector<typename T::Ptr> member_ptrs;

			void populate_pointers();
			virtual void create_member(unsigned int pattern) = 0;
	};

	/**
	 * \brief The friendly team.
	 */
	class XBeeFriendlyTeam : public GenericTeam<AI::BE::XBee::Player, AI::BE::Player, AI::BE::FriendlyTeam> {
		public:
			explicit XBeeFriendlyTeam(XBeeBackend &backend, XBeeDongle &dongle);
			unsigned int score() const;

		protected:
			void create_member(unsigned int pattern);

		private:
			XBeeDongle &dongle;
	};

	/**
	 * \brief The enemy team.
	 */
	class XBeeEnemyTeam : public GenericTeam<AI::BE::XBee::Robot, AI::BE::Robot, AI::BE::EnemyTeam> {
		public:
			explicit XBeeEnemyTeam(XBeeBackend &backend);
			unsigned int score() const;

		protected:
			void create_member(unsigned int pattern);
	};

	/**
	 * \brief The backend.
	 */
	class XBeeBackend : public Backend {
		public:
			AI::BE::XBee::RefBox refbox;

			explicit XBeeBackend(XBeeDongle &dongle, unsigned int camera_mask, unsigned int multicast_interface);
			BackendFactory &factory() const;
			const Field &field() const;
			const Ball &ball() const;
			FriendlyTeam &friendly_team();
			const FriendlyTeam &friendly_team() const;
			EnemyTeam &enemy_team();
			const EnemyTeam &enemy_team() const;
			unsigned int main_ui_controls_table_rows() const;
			void main_ui_controls_attach(Gtk::Table &, unsigned int);
			unsigned int secondary_ui_controls_table_rows() const;
			void secondary_ui_controls_attach(Gtk::Table &, unsigned int);
			std::size_t visualizable_num_robots() const;
			Visualizable::Robot::Ptr visualizable_robot(std::size_t i) const;
			void mouse_pressed(Point, unsigned int);
			void mouse_released(Point, unsigned int);
			void mouse_exited();
			void mouse_moved(Point);
			timespec monotonic_time() const;

		private:
			unsigned int camera_mask;
			TimerFDClockSource clock;
			AI::BE::XBee::Field field_;
			AI::BE::XBee::Ball ball_;
			XBeeFriendlyTeam friendly;
			XBeeEnemyTeam enemy;
			const FileDescriptor vision_socket;
			timespec playtype_time;
			Point playtype_arm_ball_position;
			SSL_DetectionFrame detections[2];
			timespec now;

			void tick();
			bool on_vision_readable(Glib::IOCondition);
			void on_refbox_packet(const void *data, std::size_t length);
			void update_playtype();
			void on_friendly_colour_changed();
			AI::Common::PlayType compute_playtype(AI::Common::PlayType old_pt);
	};

	class XBeeBackendFactory : public BackendFactory {
		public:
			explicit XBeeBackendFactory();
			void create_backend(const std::string &, unsigned int camera_mask, unsigned int multicast_interface, std::function<void(Backend &)> cb) const;
	};
}

XBeeBackendFactory xbee_backend_factory_instance;

template<typename T, typename TSuper, typename Super> GenericTeam<T, TSuper, Super>::GenericTeam(XBeeBackend &backend) : backend(backend) {
}

template<typename T, typename TSuper, typename Super> std::size_t GenericTeam<T, TSuper, Super>::size() const {
	return member_ptrs.size();
}

template<typename T, typename TSuper, typename Super> typename T::Ptr GenericTeam<T, TSuper, Super>::get_xbee_robot(std::size_t i) {
	return member_ptrs[i];
}

template<typename T, typename TSuper, typename Super> typename TSuper::Ptr GenericTeam<T, TSuper, Super>::get(std::size_t i) {
	return member_ptrs[i];
}

template<typename T, typename TSuper, typename Super> typename TSuper::CPtr GenericTeam<T, TSuper, Super>::get(std::size_t i) const {
	return member_ptrs[i];
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::clear() {
	for (std::size_t i = 0; i < members.SIZE; ++i) {
		members.destroy(i);
	}
	member_ptrs.clear();
	Super::signal_membership_changed().emit();
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts) {
	bool membership_changed = false;

	// Update existing robots and create new robots.
	std::vector<bool> used_data[2];
	for (unsigned int i = 0; i < 2; ++i) {
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
		used_data[i].resize(rep.size(), false);
		for (int j = 0; j < rep.size(); ++j) {
			const SSL_DetectionRobot &detbot = rep.Get(j);
			if (detbot.has_robot_id()) {
				unsigned int pattern = detbot.robot_id();
				typename T::Ptr bot = members[pattern];
				if (!bot) {
					create_member(pattern);
					membership_changed = true;
				}
				if (!bot->seen_this_frame) {
					bot->seen_this_frame = true;
					bot->update(detbot, ts);
				}
				used_data[i][j] = true;
			}
		}
	}

	// Count failures.
	for (std::size_t i = 0; i < members.SIZE; ++i) {
		typename T::Ptr bot = members[i];
		if (bot) {
			if (!bot->seen_this_frame) {
				++bot->vision_failures;
			} else {
				bot->vision_failures = 0;
			}
			bot->seen_this_frame = false;
			if (bot->vision_failures >= MAX_VISION_FAILURES) {
				members.destroy(i);
				membership_changed = true;
			}
		}
	}

	// If membership changed, rebuild the pointer array and emit the signal.
	if (membership_changed) {
		populate_pointers();
		Super::signal_membership_changed().emit();
	}
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::lock_time(const timespec &now) {
	for (auto i = member_ptrs.begin(), iend = member_ptrs.end(); i != iend; ++i) {
		(*i)->lock_time(now);
	}
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::populate_pointers() {
	member_ptrs.clear();
	for (std::size_t i = 0; i < members.SIZE; ++i) {
		typename T::Ptr p = members[i];
		if (p) {
			member_ptrs.push_back(p);
		}
	}
}

XBeeFriendlyTeam::XBeeFriendlyTeam(XBeeBackend &backend, XBeeDongle &dongle) : GenericTeam<AI::BE::XBee::Player, AI::BE::Player, AI::BE::FriendlyTeam>(backend), dongle(dongle) {
}

unsigned int XBeeFriendlyTeam::score() const {
	return backend.friendly_colour() == AI::Common::Team::Colour::YELLOW ? backend.refbox.goals_yellow : backend.refbox.goals_blue;
}

void XBeeFriendlyTeam::create_member(unsigned int pattern) {
	members.create(pattern, std::ref(backend), pattern, std::ref(dongle.robot(pattern)));
}

XBeeEnemyTeam::XBeeEnemyTeam(XBeeBackend &backend) : GenericTeam<AI::BE::XBee::Robot, AI::BE::Robot, AI::BE::EnemyTeam>(backend) {
}

unsigned int XBeeEnemyTeam::score() const {
	return backend.friendly_colour() == AI::Common::Team::Colour::YELLOW ? backend.refbox.goals_blue : backend.refbox.goals_yellow;
}

void XBeeEnemyTeam::create_member(unsigned int pattern) {
	members.create(pattern, std::ref(backend), pattern);
}

XBeeBackend::XBeeBackend(XBeeDongle &dongle, unsigned int camera_mask, unsigned int multicast_interface) : Backend(), refbox(multicast_interface), camera_mask(camera_mask), clock(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND), ball_(*this), friendly(*this, dongle), enemy(*this), vision_socket(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
	if (!(1 <= camera_mask && camera_mask <= 3)) {
		throw std::runtime_error("Invalid camera bitmask (must be 1–3)");
	}

	refbox.command.signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_playtype));
	friendly_colour().signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::on_friendly_colour_changed));
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_playtype));

	clock.signal_tick.connect(sigc::mem_fun(this, &XBeeBackend::tick));

	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	AddrInfoSet ai(0, "10002", &hints);

	vision_socket.set_blocking(false);

	const int one = 1;
	if (setsockopt(vision_socket.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw SystemError("setsockopt(SO_REUSEADDR)", errno);
	}

	if (bind(vision_socket.fd(), ai.first()->ai_addr, ai.first()->ai_addrlen) < 0) {
		throw SystemError("bind(:10002)", errno);
	}

	ip_mreqn mcreq;
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
	mcreq.imr_address.s_addr = get_inaddr_any();
	mcreq.imr_ifindex = multicast_interface;
	if (setsockopt(vision_socket.fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG_INFO("Cannot join multicast group 224.5.23.2 for vision data.");
	}

	Glib::signal_io().connect(sigc::mem_fun(this, &XBeeBackend::on_vision_readable), vision_socket.fd(), Glib::IO_IN);

	refbox.signal_packet.connect(sigc::mem_fun(this, &XBeeBackend::on_refbox_packet));
	refbox.goals_yellow.signal_changed().connect(signal_score_changed().make_slot());
	refbox.goals_blue.signal_changed().connect(signal_score_changed().make_slot());

	timespec_now(playtype_time);
}

BackendFactory &XBeeBackend::factory() const {
	return xbee_backend_factory_instance;
}

const Field &XBeeBackend::field() const {
	return field_;
}

const Ball &XBeeBackend::ball() const {
	return ball_;
}

FriendlyTeam &XBeeBackend::friendly_team() {
	return friendly;
}

const FriendlyTeam &XBeeBackend::friendly_team() const {
	return friendly;
}

EnemyTeam &XBeeBackend::enemy_team() {
	return enemy;
}

const EnemyTeam &XBeeBackend::enemy_team() const {
	return enemy;
}

unsigned int XBeeBackend::main_ui_controls_table_rows() const {
	return 0;
}

void XBeeBackend::main_ui_controls_attach(Gtk::Table &, unsigned int) {
}

unsigned int XBeeBackend::secondary_ui_controls_table_rows() const {
	return 0;
}

void XBeeBackend::secondary_ui_controls_attach(Gtk::Table &, unsigned int) {
}

std::size_t XBeeBackend::visualizable_num_robots() const {
	return friendly.size() + enemy.size();
}

Visualizable::Robot::Ptr XBeeBackend::visualizable_robot(std::size_t i) const {
	if (i < friendly.size()) {
		return friendly.get(i);
	} else {
		return enemy.get(i - friendly.size());
	}
}

void XBeeBackend::mouse_pressed(Point, unsigned int) {
}

void XBeeBackend::mouse_released(Point, unsigned int) {
}

void XBeeBackend::mouse_exited() {
}

void XBeeBackend::mouse_moved(Point) {
}

timespec XBeeBackend::monotonic_time() const {
	return now;
}

void XBeeBackend::tick() {
	// If the field geometry is not yet valid, do nothing.
	if (!field_.valid()) {
		return;
	}

	// Do pre-AI stuff (locking predictors).
	timespec_now(now);
	ball_.lock_time(now);
	friendly.lock_time(now);
	enemy.lock_time(now);
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		friendly.get_xbee_robot(i)->pre_tick();
	}
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy.get_xbee_robot(i)->pre_tick();
	}

	// Run the AI.
	signal_tick().emit();

	// Do post-AI stuff (pushing data to the radios).
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		friendly.get_xbee_robot(i)->tick(playtype() == AI::Common::PlayType::HALT);
	}

	// Notify anyone interested in the finish of a tick.
	timespec after;
	timespec_now(after);
	signal_post_tick().emit(timespec_to_nanos(timespec_sub(after, now)));
}

bool XBeeBackend::on_vision_readable(Glib::IOCondition) {
	// Receive a packet.
	uint8_t buffer[65536];
	ssize_t len = recv(vision_socket.fd(), buffer, sizeof(buffer), 0);
	if (len < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG_WARN("Cannot receive packet from SSL-Vision.");
		}
		return true;
	}

	// Decode it.
	SSL_WrapperPacket packet;
	if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
		LOG_WARN("Received malformed SSL-Vision packet.");
		return true;
	}

	// Pass it to any attached listeners.
	timespec now;
	timespec_now(now);
	signal_vision().emit(now, packet);

	// If it contains geometry data, update the field shape.
	if (packet.has_geometry()) {
		const SSL_GeometryData &geom(packet.geometry());
		const SSL_GeometryFieldSize &fsize(geom.field());
		field_.update(fsize);
	}

	// If it contains ball and robot data, update the ball and the teams.
	if (packet.has_detection()) {
		const SSL_DetectionFrame &det(packet.detection());

		// Check for a sensible camera ID number.
		if (det.camera_id() >= 2) {
			LOG_WARN(Glib::ustring::compose("Received SSL-Vision packet for unknown camera %1.", det.camera_id()));
			return true;
		}

		// Check for an accepted camera ID number.
		if (!(camera_mask & (1U << det.camera_id()))) {
			return true;
		}

		// Keep a local copy of all detection frames.
		detections[det.camera_id()].CopyFrom(det);

		// Take a timestamp.
		timespec now;
		timespec_now(now);

		// Update the ball.
		{
			// Build a vector of all detections so far.
			std::vector<std::pair<double, Point> > balls;
			for (unsigned int i = 0; i < 2; ++i) {
				for (int j = 0; j < detections[i].balls_size(); ++j) {
					const SSL_DetectionBall &b(detections[i].balls(j));
					balls.push_back(std::make_pair(b.confidence(), Point(b.x() / 1000.0, b.y() / 1000.0)));
				}
			}

			// Execute the current ball filter.
			Point pos;
			if (ball_filter()) {
				pos = ball_filter()->filter(balls, *this);
			}

			// Use the result.
			ball_.update(pos, now);
		}

		// Update the robots.
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *yellow_packets[2] = { &detections[0].robots_yellow(), &detections[1].robots_yellow() };
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *blue_packets[2] = { &detections[0].robots_blue(), &detections[1].robots_blue() };
		if (friendly_colour() == AI::Common::Team::Colour::YELLOW) {
			friendly.update(yellow_packets, now);
			enemy.update(blue_packets, now);
		} else {
			friendly.update(blue_packets, now);
			enemy.update(yellow_packets, now);
		}
	}

	// Movement of the ball may, potentially, result in a play type change.
	update_playtype();

	return true;
}

void XBeeBackend::on_refbox_packet(const void *data, std::size_t length) {
	timespec now;
	timespec_now(now);
	signal_refbox().emit(now, data, length);
}

void XBeeBackend::update_playtype() {
	AI::Common::PlayType new_pt;
	AI::Common::PlayType old_pt = playtype();
	if (playtype_override() != AI::Common::PlayType::NONE) {
		new_pt = playtype_override();
	} else {
		if (friendly_colour() == AI::Common::Team::Colour::YELLOW) {
			old_pt = AI::Common::PlayTypeInfo::invert(old_pt);
		}
		new_pt = compute_playtype(old_pt);
		if (friendly_colour() == AI::Common::Team::Colour::YELLOW) {
			new_pt = AI::Common::PlayTypeInfo::invert(new_pt);
		}
	}
	if (new_pt != playtype()) {
		playtype_rw() = new_pt;
		timespec_now(playtype_time);
	}
}

void XBeeBackend::on_friendly_colour_changed() {
	update_playtype();
	friendly.clear();
	enemy.clear();
}

AI::Common::PlayType XBeeBackend::compute_playtype(AI::Common::PlayType old_pt) {
	switch (refbox.command) {
		case 'H': // HALT
		case 'h': // HALF TIME
		case 't': // TIMEOUT YELLOW
		case 'T': // TIMEOUT BLUE
			return AI::Common::PlayType::HALT;

		case 'S': // STOP
		case 'z': // END TIMEOUT
			return AI::Common::PlayType::STOP;

		case ' ': // NORMAL START
			switch (old_pt) {
				case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
					playtype_arm_ball_position = ball_.position();
					return AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY;

				case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
					playtype_arm_ball_position = ball_.position();
					return AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY;

				case AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY:
					playtype_arm_ball_position = ball_.position();
					return AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY;

				case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
					playtype_arm_ball_position = ball_.position();
					return AI::Common::PlayType::EXECUTE_PENALTY_ENEMY;

				case AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY:
				case AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
					if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
						return AI::Common::PlayType::PLAY;
					} else {
						return old_pt;
					}

				default:
					return AI::Common::PlayType::PLAY;
			}

		case 'f': // DIRECT FREE KICK YELLOW
			if (old_pt == AI::Common::PlayType::PLAY) {
				return AI::Common::PlayType::PLAY;
			} else if (old_pt == AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return AI::Common::PlayType::PLAY;
				} else {
					return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
			}

		case 'F': // DIRECT FREE KICK BLUE
			if (old_pt == AI::Common::PlayType::PLAY) {
				return AI::Common::PlayType::PLAY;
			} else if (old_pt == AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return AI::Common::PlayType::PLAY;
				} else {
					return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
			}

		case 'i': // INDIRECT FREE KICK YELLOW
			if (old_pt == AI::Common::PlayType::PLAY) {
				return AI::Common::PlayType::PLAY;
			} else if (old_pt == AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return AI::Common::PlayType::PLAY;
				} else {
					return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
			}

		case 'I': // INDIRECT FREE KICK BLUE
			if (old_pt == AI::Common::PlayType::PLAY) {
				return AI::Common::PlayType::PLAY;
			} else if (old_pt == AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return AI::Common::PlayType::PLAY;
				} else {
					return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
			}

		case 's': // FORCE START
			return AI::Common::PlayType::PLAY;

		case 'k': // KICKOFF YELLOW
			return AI::Common::PlayType::PREPARE_KICKOFF_ENEMY;

		case 'K': // KICKOFF BLUE
			return AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY;

		case 'p': // PENALTY YELLOW
			return AI::Common::PlayType::PREPARE_PENALTY_ENEMY;

		case 'P': // PENALTY BLUE
			return AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY;

		case '1': // BEGIN FIRST HALF
		case '2': // BEGIN SECOND HALF
		case 'o': // BEGIN OVERTIME 1
		case 'O': // BEGIN OVERTIME 2
		case 'a': // BEGIN PENALTY SHOOTOUT
		case 'g': // GOAL YELLOW
		case 'G': // GOAL BLUE
		case 'd': // REVOKE GOAL YELLOW
		case 'D': // REVOKE GOAL BLUE
		case 'y': // YELLOW CARD YELLOW
		case 'Y': // YELLOW CARD BLUE
		case 'r': // RED CARD YELLOW
		case 'R': // RED CARD BLUE
		case 'c': // CANCEL
		default:
			return old_pt;
	}
}

XBeeBackendFactory::XBeeBackendFactory() : BackendFactory("xbee") {
}

void XBeeBackendFactory::create_backend(const std::string &, unsigned int camera_mask, unsigned int multicast_interface, std::function<void(Backend &)> cb) const {
	XBeeDongle dongle;
	dongle.enable();
	XBeeBackend be(dongle, camera_mask, multicast_interface);
	cb(be);
}

