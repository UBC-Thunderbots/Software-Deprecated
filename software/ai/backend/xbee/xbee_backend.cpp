#include "ai/backend/xbee/xbee_backend.h"
#include "ai/backend/backend.h"
#include "ai/backend/xbee/ball.h"
#include "ai/backend/xbee/field.h"
#include "ai/backend/xbee/player.h"
#include "ai/backend/xbee/refbox.h"
#include "ai/backend/xbee/robot.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

DoubleParam LOOP_DELAY("Loop Delay","Backend/XBee",0.0,-1.0,1.0);

using namespace AI::BE;

namespace {
	/**
	 * The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
	 */
	const double BALL_FREE_DISTANCE = 0.09;

	/**
	 * The number of vision failures to tolerate before assuming the robot is gone and removing it from the system.
	 * Note that this should be fairly high because the failure count includes instances of a packet arriving from a camera that cannot see the robot
	 * (this is expected to cause a failure to be counted which will then be zeroed out a moment later as the other camera sends its packet).
	 */
	const unsigned int MAX_VISION_FAILURES = 120;

	class XBeeBackend;

	/**
	 * A generic team.
	 *
	 * \tparam T the type of robot on this team, either Player or Robot.
	 */
	template<typename T> class GenericTeam {
		public:
			GenericTeam(XBeeBackend &backend);
			~GenericTeam();
			void clear();
			void update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts);
			void lock_time(const timespec &now);
			virtual std::size_t size() const = 0;

		protected:
			XBeeBackend &backend;
			std::vector<typename T::Ptr> members;

			virtual typename T::Ptr create_member(unsigned int pattern) = 0;
			virtual sigc::signal<void, std::size_t> &signal_robot_added() const = 0;
			virtual sigc::signal<void, std::size_t> &signal_robot_removing() const = 0;
			virtual sigc::signal<void> &signal_robot_removed() const = 0;
	};

	template<typename T> GenericTeam<T>::GenericTeam(XBeeBackend &backend) : backend(backend) {
	}

	template<typename T> GenericTeam<T>::~GenericTeam() {
	}

	/**
	 * The friendly team.
	 */
	class XBeeFriendlyTeam : public FriendlyTeam, public GenericTeam<AI::BE::XBee::Player> {
		public:
			XBeeFriendlyTeam(XBeeBackend &backend, XBeeDongle &dongle);
			~XBeeFriendlyTeam();
			unsigned int score() const;
			std::size_t size() const;
			Player::Ptr get(std::size_t i) { return members[i]; }
			Player::CPtr get(std::size_t i) const { return members[i]; }
			AI::BE::XBee::Player::Ptr create_member(unsigned int pattern);
			AI::BE::XBee::Player::Ptr get_xbee_player(std::size_t i) { return members[i]; }

		private:
			XBeeDongle &dongle;

			sigc::signal<void, std::size_t> &signal_robot_added() const { return FriendlyTeam::signal_robot_added(); }
			sigc::signal<void, std::size_t> &signal_robot_removing() const { return FriendlyTeam::signal_robot_removing(); }
			sigc::signal<void> &signal_robot_removed() const { return FriendlyTeam::signal_robot_removed(); }
	};

	/**
	 * The enemy team.
	 */
	class XBeeEnemyTeam : public EnemyTeam, public GenericTeam<AI::BE::XBee::Robot> {
		public:
			XBeeEnemyTeam(XBeeBackend &backend);
			~XBeeEnemyTeam();
			unsigned int score() const;
			std::size_t size() const;
			Robot::Ptr get(std::size_t i) const { return members[i]; }
			AI::BE::XBee::Robot::Ptr create_member(unsigned int pattern);

		private:
			sigc::signal<void, std::size_t> &signal_robot_added() const { return EnemyTeam::signal_robot_added(); }
			sigc::signal<void, std::size_t> &signal_robot_removing() const { return EnemyTeam::signal_robot_removing(); }
			sigc::signal<void> &signal_robot_removed() const { return EnemyTeam::signal_robot_removed(); }
	};

	/**
	 * The backend.
	 */
	class XBeeBackend : public Backend {
		public:
			AI::BE::XBee::RefBox refbox;

			XBeeBackend(XBeeDongle &dongle) : Backend(), clock(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND), ball_(*this), friendly(*this, dongle), enemy(*this), vision_socket(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
				refbox.command.signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_playtype));
				friendly_colour().signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::on_friendly_colour_changed));
				playtype_override().signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_playtype));

				clock.signal_tick.connect(sigc::mem_fun(this, &XBeeBackend::tick));

				vision_socket->set_blocking(false);
				const int one = 1;
				if (setsockopt(vision_socket->fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
					throw SystemError("setsockopt(SO_REUSEADDR)", errno);
				}
				SockAddrs sa;
				sa.in.sin_family = AF_INET;
				sa.in.sin_addr.s_addr = get_inaddr_any();
				encode_u16(&sa.in.sin_port, 10002);
				std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
				if (bind(vision_socket->fd(), &sa.sa, sizeof(sa.in)) < 0) {
					throw SystemError("bind(:10002)", errno);
				}
				ip_mreqn mcreq;
				mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
				mcreq.imr_address.s_addr = get_inaddr_any();
				mcreq.imr_ifindex = 0;
				if (setsockopt(vision_socket->fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
					LOG_INFO("Cannot join multicast group 224.5.23.2 for vision data.");
				}
				Glib::signal_io().connect(sigc::mem_fun(this, &XBeeBackend::on_vision_readable), vision_socket->fd(), Glib::IO_IN);

				refbox.signal_packet.connect(signal_refbox().make_slot());
				refbox.goals_yellow.signal_changed().connect(signal_score_changed().make_slot());
				refbox.goals_blue.signal_changed().connect(signal_score_changed().make_slot());

				timespec_now(playtype_time);
			}

			~XBeeBackend() {
			}

			BackendFactory &factory() const;

			const Field &field() const {
				return field_;
			}

			const Ball &ball() const {
				return ball_;
			}

			FriendlyTeam &friendly_team() {
				return friendly;
			}

			const FriendlyTeam &friendly_team() const {
				return friendly;
			}

			const EnemyTeam &enemy_team() const {
				return enemy;
			}

			unsigned int ui_controls_table_rows() const {
				return 0;
			}

			void ui_controls_attach(Gtk::Table &, unsigned int) {
			}

			std::size_t visualizable_num_robots() const {
				return friendly.size() + enemy.size();
			}

			Visualizable::Robot::Ptr visualizable_robot(std::size_t i) const {
				if (i < friendly.size()) {
					return friendly.get(i);
				} else {
					return enemy.get(i - friendly.size());
				}
			}

			void mouse_pressed(Point, unsigned int) {
			}

			void mouse_released(Point, unsigned int) {
			}

			void mouse_exited() {
			}

			void mouse_moved(Point) {
			}

			timespec monotonic_time() const {
				return now;
			}

		private:
			TimerFDClockSource clock;
			AI::BE::XBee::Field field_;
			AI::BE::XBee::Ball ball_;
			XBeeFriendlyTeam friendly;
			XBeeEnemyTeam enemy;
			const FileDescriptor::Ptr vision_socket;
			timespec playtype_time;
			Point playtype_arm_ball_position;
			SSL_DetectionFrame detections[2];
			timespec now;

			void tick() {
				// If the field geometry is not yet valid, do nothing.
				if (!field_.valid()) {
					return;
				}

				// Do pre-AI stuff (locking predictors).
				timespec_now(now);
				ball_.lock_time(now);
				friendly.lock_time(now);
				enemy.lock_time(now);

				// Run the AI.
				signal_tick().emit();

				// Do post-AI stuff (pushing data to the radios).
				for (std::size_t i = 0; i < friendly.size(); ++i) {
					friendly.get_xbee_player(i)->tick(playtype() == AI::Common::PlayType::HALT);
				}

				// Notify anyone interested in the finish of a tick.
				signal_post_tick().emit();
			}

			bool on_vision_readable(Glib::IOCondition) {
				// Receive a packet.
				uint8_t buffer[65536];
				ssize_t len = recv(vision_socket->fd(), buffer, sizeof(buffer), 0);
				if (len < 0) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						LOG_WARN("Cannot receive packet from SSL-Vision.");
					}
					return true;
				}

				// Pass it to any attached listeners.
				signal_vision().emit(buffer, len);

				// Decode it.
				SSL_WrapperPacket packet;
				if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
					LOG_WARN("Received malformed SSL-Vision packet.");
					return true;
				}

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
					if (friendly_colour() == AI::Common::Team::YELLOW) {
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

			void update_playtype() {
				AI::Common::PlayType::PlayType new_pt;
				AI::Common::PlayType::PlayType old_pt = playtype();
				if (playtype_override() != AI::Common::PlayType::COUNT) {
					new_pt = playtype_override();
				} else {
					if (friendly_colour() == AI::Common::Team::YELLOW) {
						old_pt = AI::Common::PlayType::INVERT[old_pt];
					}
					new_pt = compute_playtype(old_pt);
					if (friendly_colour() == AI::Common::Team::YELLOW) {
						new_pt = AI::Common::PlayType::INVERT[new_pt];
					}
				}
				if (new_pt != playtype()) {
					playtype_rw() = new_pt;
					timespec_now(playtype_time);
				}
			}

			void on_friendly_colour_changed() {
				update_playtype();
				friendly.clear();
				enemy.clear();
			}

			AI::Common::PlayType::PlayType compute_playtype(AI::Common::PlayType::PlayType old_pt) {
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
	};

	template<typename T> void GenericTeam<T>::clear() {
		while (size()) {
			typename T::Ptr bot = members[0];
			signal_robot_removing().emit(0);
			bot->object_store().clear();
			bot.reset();
			members.erase(members.begin());
			signal_robot_removed().emit();
		}
	}

	template<typename T> void GenericTeam<T>::update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts) {
		// Update existing robots.
		std::vector<bool> used_data[2];
		for (unsigned int i = 0; i < 2; ++i) {
			const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
			used_data[i].resize(rep.size(), false);
			for (int j = 0; j < rep.size(); ++j) {
				const SSL_DetectionRobot &detbot = rep.Get(j);
				if (detbot.has_robot_id()) {
					unsigned int pattern = detbot.robot_id();
					for (std::size_t k = 0; k < size(); ++k) {
						typename T::Ptr bot = members[k];
						if (bot->pattern() == pattern) {
							if (!bot->seen_this_frame) {
								bot->seen_this_frame = true;
								bot->update(detbot, ts);
							}
							used_data[i][j] = true;
						}
					}
				}
			}
		}

		// Count failures.
		for (std::size_t i = 0; i < size(); ++i) {
			typename T::Ptr bot = members[i];
			if (!bot->seen_this_frame) {
				++bot->vision_failures;
			} else {
				bot->vision_failures = 0;
			}
			bot->seen_this_frame = false;
			if (bot->vision_failures >= MAX_VISION_FAILURES) {
				signal_robot_removing().emit(i);
				bot->object_store().clear();
				bot.reset();
				members.erase(members.begin() + i);
				signal_robot_removed().emit();
				--i;
			}
		}

		// Look for new robots and create them.
		for (unsigned int i = 0; i < 2; ++i) {
			const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
			for (int j = 0; j < rep.size(); ++j) {
				if (!used_data[i][j]) {
					const SSL_DetectionRobot &detbot(rep.Get(j));
					if (detbot.has_robot_id()) {
						typename T::Ptr bot = create_member(detbot.robot_id());
						if (bot.is()) {
							unsigned int k;
							for (k = 0; k < members.size() && members[k]->pattern() < detbot.robot_id(); ++k) {
							}
							members.insert(members.begin() + k, bot);
							signal_robot_added().emit(k);
						}
					}
				}
			}
		}
	}

	template<typename T> void GenericTeam<T>::lock_time(const timespec &now) {
		for (typename std::vector<typename T::Ptr>::const_iterator i = members.begin(), iend = members.end(); i != iend; ++i) {
			(*i)->lock_time(now);
		}
	}

	XBeeFriendlyTeam::XBeeFriendlyTeam(XBeeBackend &backend, XBeeDongle &dongle) : GenericTeam<AI::BE::XBee::Player>(backend), dongle(dongle) {
	}

	XBeeFriendlyTeam::~XBeeFriendlyTeam() {
	}

	unsigned int XBeeFriendlyTeam::score() const {
		return backend.friendly_colour() == AI::Common::Team::YELLOW ? backend.refbox.goals_yellow : backend.refbox.goals_blue;
	}

	std::size_t XBeeFriendlyTeam::size() const {
		return members.size();
	}

	AI::BE::XBee::Player::Ptr XBeeFriendlyTeam::create_member(unsigned int pattern) {
		return AI::BE::XBee::Player::create(backend, pattern, dongle.robot(pattern));
	}

	XBeeEnemyTeam::XBeeEnemyTeam(XBeeBackend &backend) : GenericTeam<AI::BE::XBee::Robot>(backend) {
	}

	XBeeEnemyTeam::~XBeeEnemyTeam() {
	}

	unsigned int XBeeEnemyTeam::score() const {
		return backend.friendly_colour() == AI::Common::Team::YELLOW ? backend.refbox.goals_blue : backend.refbox.goals_yellow;
	}

	std::size_t XBeeEnemyTeam::size() const {
		return members.size();
	}

	AI::BE::XBee::Robot::Ptr XBeeEnemyTeam::create_member(unsigned int pattern) {
		return AI::BE::XBee::Robot::create(backend, pattern);
	}

	class XBeeBackendFactory : public BackendFactory {
		public:
			XBeeBackendFactory() : BackendFactory("xbee") {
			}

			~XBeeBackendFactory() {
			}

			void create_backend(const std::multimap<Glib::ustring, Glib::ustring> &params, sigc::slot<void, Backend &> cb) const {
				if (!params.empty()) {
					throw std::runtime_error("The XBee backend does not accept any parameters.");
				}
				XBeeDongle dongle;
				Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();
				dongle.enable()->signal_done.connect(sigc::bind(&XBeeBackendFactory::on_dongle_enabled, main_loop));
				main_loop->run();
				XBeeBackend be(dongle);
				cb(be);
			}

		private:
			static void on_dongle_enabled(AsyncOperation<void>::Ptr op, Glib::RefPtr<Glib::MainLoop> main_loop) {
				op->result();
				main_loop->quit();
			}
	};

	XBeeBackendFactory factory_instance;

	BackendFactory &XBeeBackend::factory() const {
		return factory_instance;
	}
}

