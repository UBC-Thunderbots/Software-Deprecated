#include "datapool/Config.h"
#include "datapool/Field.h"
#include "datapool/Hungarian.h"
#include "datapool/Noncopyable.h"
#include "datapool/Player.h"
#include "datapool/Team.h"
#include "datapool/World.h"
#include "IR/ImageRecognition.h"
#include "IR/messages_robocup_ssl_detection.pb.h"
#include "IR/messages_robocup_ssl_geometry.pb.h"
#include "IR/messages_robocup_ssl_wrapper.pb.h"
#include "Log/Log.h"

#include <vector>
#include <queue>
#include <list>
#include <utility>
#include <algorithm>
#include <limits>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <glibmm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define OFFSET_FROM_ROBOT_TO_BALL 80
#define ANTIFLICKER_FRAMES        60

namespace {
	SSL_DetectionFrame detections[2];

	class AntiFlickerCircle;
	std::list<AntiFlickerCircle *> antiFlickerCircles;
	std::queue<std::vector<std::pair<AntiFlickerCircle *, unsigned int> > > antiFlickerCirclesByFrame;
	class AntiFlickerCircle : public virtual Noncopyable {
	public:
		AntiFlickerCircle(const Vector2 &centre) : cen(centre), pop(0), parent(0), iter(antiFlickerCircles.insert(antiFlickerCircles.end(), this)) {
		}

		~AntiFlickerCircle() {
			assert(!pop);
			antiFlickerCircles.erase(iter);
		}

		bool isInside(const Vector2 &pt) const {
			return (pt - cen).length() < ImageRecognition::ANTIFLICKER_CIRCLE_RADIUS;
		}

		AntiFlickerCircle *representative() {
			if (parent) {
				AntiFlickerCircle *rep = parent->representative();
				if (rep != parent) {
					rep->addRefs(pop);
					parent->subRefs(pop);
					parent = rep;
				}
				return rep;
			} else {
				return this;
			}
		}

		const AntiFlickerCircle *representative() const {
			if (parent) {
				AntiFlickerCircle *rep = parent->representative();
				if (rep != parent) {
					rep->addRefs(pop);
					parent->subRefs(pop);
					parent = rep;
				}
				return rep;
			} else {
				return this;
			}
		}

		void merge(AntiFlickerCircle *other) {
			AntiFlickerCircle *me = representative();
			other = other->representative();
			if (me != other) {
				me->parent = other;
				other->pop += me->pop;
			}
		}

		unsigned int popularity() const {
			return representative()->pop;
		}

		const Vector2 &centre() const {
			return cen;
		}

		Vector2 &centre() {
			return cen;
		}

		void addRefs(unsigned int n) {
			assert(pop < UINT_MAX - n + 1);
			pop += n;
			if (parent)
				parent->addRefs(n);
		}

		void subRefs(unsigned int n) {
			assert(pop >= n);
			pop -= n;
			if (parent)
				parent->subRefs(n);
			if (!pop)
				delete this;
		}

		const SSL_DetectionBall *bestBall;
		unsigned int bestBallConfidence;
		unsigned int inFrameIndex;

	private:
		Vector2 cen;
		unsigned int pop;
		mutable AntiFlickerCircle *parent;
		std::list<AntiFlickerCircle *>::iterator iter;
	};
}

ImageRecognition::ImageRecognition(Team &friendly, Team &enemy) : fd(-1) {
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot create socket: " << std::strerror(err) << '\n';
		return;
	}

	int reuse = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot set socket option: " << std::strerror(err) << '\n';
		fd = -1;
		return;
	}

	union {
		sockaddr sa;
		sockaddr_in in;
	} sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.in.sin_port = htons(10002);
	std::memset(&sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (::bind(fd, &sa.sa, sizeof(sa.in)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot bind: " << std::strerror(err) << '\n';
		fd = -1;
		return;
	}

	ip_mreqn req;
	req.imr_multiaddr.s_addr = ::inet_addr("224.5.23.2");
	req.imr_address.s_addr = htonl(INADDR_ANY);
	req.imr_ifindex = 0;
	if (::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot join multicast group: " << std::strerror(err) << '\n';
		fd = -1;
		return;
	}

	Field field;
	field.width(660);
	field.height(470);
	field.west(25);
	field.east(635);
	field.north(25);
	field.south(445);
	field.centerCircle(Vector2(330, 235));
	field.centerCircleRadius(50);
	field.infinity(1e9);

	field.westGoal().north.x = 25;
	field.westGoal().north.y = 200;
	field.westGoal().south.x = 25;
	field.westGoal().south.y = 270;
	field.westGoal().defenseN.x = 75;
	field.westGoal().defenseN.y = 217.5;
	field.westGoal().defenseS.x = 75;
	field.westGoal().defenseS.y = 252.5;
	field.westGoal().height = 16;
	field.westGoal().penalty.x = 70;
	field.westGoal().penalty.y = 235;

	field.eastGoal().north.x = 635;
	field.eastGoal().north.y = 200;
	field.eastGoal().south.x = 635;
	field.eastGoal().south.y = 270;
	field.eastGoal().defenseN.x = 585;
	field.eastGoal().defenseN.y = 217.5;
	field.eastGoal().defenseS.x = 585;
	field.eastGoal().defenseS.y = 252.5;
	field.eastGoal().height = 16;
	field.eastGoal().penalty.x = 590;
	field.eastGoal().penalty.y = 235;

	World::init(friendly, enemy, field);

	World &w = World::get();

	//Set the player properties:
	const double infinity = World::get().field().infinity();
	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		w.player(i)->position(Vector2(infinity, infinity));
		w.player(i)->radius(90);
	}

	//Set the ball properties:
	w.ball().position(Vector2(330, 235));
	w.ball().radius(21.5);

	// Register for IO.
	Glib::signal_io().connect(sigc::mem_fun(*this, &ImageRecognition::onIO), fd, Glib::IO_IN);

	// Register for quarter-second ticks.
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &ImageRecognition::onTimer), 250);
}

bool ImageRecognition::onIO(Glib::IOCondition cond) {
	if (fd < 0)
		return false;

	if (cond & (Glib::IO_ERR | Glib::IO_NVAL | Glib::IO_HUP)) {
		Log::log(Log::LEVEL_ERROR, "IR") << "Error polling SSL-Vision socket.\n";
		return true;
	}

	if (!(cond & Glib::IO_IN))
		return true;

	char buffer[65536];
	ssize_t ret;
	ret = recv(fd, buffer, sizeof(buffer), 0);
	if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return true;
	} else if (ret < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot receive data: " << std::strerror(err) << '\n';
		return true;
	}

	SSL_WrapperPacket pkt;
	pkt.ParseFromArray(buffer, ret);
	if (pkt.has_geometry()) {
		const SSL_GeometryFieldSize &fData = pkt.geometry().field();

		Goal &goalW = World::get().field().westGoal();
		goalW.north = Vector2(-fData.field_length() / 2.0, -fData.goal_width() / 2.0);
		goalW.south = Vector2(-fData.field_length() / 2.0, fData.goal_width() / 2.0);
		goalW.defenseN = Vector2(-fData.field_length() / 2.0 + fData.defense_radius(), -fData.defense_stretch() / 2.0);
		goalW.defenseS = Vector2(-fData.field_length() / 2.0 + fData.defense_radius(), fData.defense_stretch() / 2.0);
		goalW.height = 160;
		goalW.penalty = Vector2(-fData.field_length() / 2.0 + 450, 0);

		Goal &goalE = World::get().field().eastGoal();
		goalE.north = Vector2(fData.field_length() / 2.0, -fData.goal_width() / 2.0);
		goalE.south = Vector2(fData.field_length() / 2.0, fData.goal_width() / 2.0);
		goalE.defenseN = Vector2(fData.field_length() / 2.0 - fData.defense_radius(), -fData.defense_stretch() / 2.0);
		goalE.defenseS = Vector2(fData.field_length() / 2.0 - fData.defense_radius(), fData.defense_stretch() / 2.0);
		goalE.height = 160;
		goalE.penalty = Vector2(fData.field_length() / 2.0 - 450, 0);

		Field &field = World::get().field();
		field.width(fData.field_length());
		field.height(fData.field_width());
		field.west(-fData.field_length() / 2.0);
		field.east(fData.field_length() / 2.0);
		field.north(-fData.field_width() / 2.0);
		field.south(fData.field_width() / 2.0);
		field.centerCircle(Vector2(0, 0));
		field.centerCircleRadius(fData.center_circle_radius());
	}

	if (pkt.has_detection()) {
		detections[pkt.detection().camera_id()] = pkt.detection();

		// Handle the ball.
		{
			// Clear all the circle balls.
			for (std::list<AntiFlickerCircle *>::iterator i = antiFlickerCircles.begin(), iEnd = antiFlickerCircles.end(); i != iEnd; ++i) {
				AntiFlickerCircle *c = *i;
				assert(c);
				c->bestBall = 0;
				c->bestBallConfidence = 0;
				c->inFrameIndex = 0;
			}

			// Start a new frame.
			antiFlickerCirclesByFrame.push(std::vector<std::pair<AntiFlickerCircle *, unsigned int> >());

			// Go through all the detected balls, associating them with circles.
			for (unsigned int i = 0; i < sizeof(detections) / sizeof(*detections); i++) {
				for (int j = 0; j < detections[i].balls_size(); j++) {
					const SSL_DetectionBall &b = detections[i].balls(j);
					assert(&b);
					const unsigned int cnt = b.confidence() * 100;
					assert(cnt);
					const Vector2 pos(b.x(), -b.y());
					if (b.confidence() > 0.01) {
						// Add this ball to all circles it's inside.
						bool found = false;
						for (std::list<AntiFlickerCircle *>::iterator k = antiFlickerCircles.begin(), kEnd = antiFlickerCircles.end(); k != kEnd; ++k) {
							AntiFlickerCircle &c = **k;
							assert(&c);
							if (c.isInside(pos)) {
								found = true;
								if (!c.bestBall || c.bestBall->confidence() < b.confidence()) {
									if (c.bestBall) {
										c.addRefs(cnt);
										c.subRefs(c.bestBallConfidence);
										std::pair<AntiFlickerCircle *, unsigned int> &e = antiFlickerCirclesByFrame.back()[c.inFrameIndex];
										assert(e.first);
										assert(e.second);
										e.second += cnt;
										e.second -= c.bestBallConfidence;
										assert(e.second);
										c.bestBall = &b;
										c.bestBallConfidence = cnt;
									} else {
										c.addRefs(cnt);
										c.inFrameIndex = antiFlickerCirclesByFrame.back().size();
										antiFlickerCirclesByFrame.back().push_back(std::make_pair(&c, cnt));
										c.bestBall = &b;
										c.bestBallConfidence = cnt;
									}
								}
							}
						}

						// If no circles contained the ball, create a new one.
						if (!found) {
							AntiFlickerCircle *c = new AntiFlickerCircle(pos);
							c->addRefs(cnt);
							c->bestBall = &b;
							c->bestBallConfidence = cnt;
							c->inFrameIndex = antiFlickerCirclesByFrame.back().size();
							antiFlickerCirclesByFrame.back().push_back(std::make_pair(c, cnt));
						}
					}
				}
			}

			// Delete past frames.
			while (antiFlickerCirclesByFrame.size() > ANTIFLICKER_FRAMES) {
				const std::vector<std::pair<AntiFlickerCircle *, unsigned int> > &v = antiFlickerCirclesByFrame.front();
				for (unsigned int i = 0; i < v.size(); i++)
					v[i].first->subRefs(v[i].second);
				antiFlickerCirclesByFrame.pop();
			}

			// Merge overlapping circles.
			for (unsigned int i = 0; i < sizeof(detections) / sizeof(*detections); i++) {
				for (int j = 0; j < detections[i].balls_size(); j++) {
					const SSL_DetectionBall &b = detections[i].balls(j);
					if (b.confidence() > 0.01) {
						// Find all circles for which this ball is the best. Merge them.
						AntiFlickerCircle *prevCircle = 0;
						for (std::list<AntiFlickerCircle *>::iterator k = antiFlickerCircles.begin(), kEnd = antiFlickerCircles.end(); k != kEnd; ++k) {
							AntiFlickerCircle &c = **k;
							assert(&c);
							if (c.bestBall == &b) {
								if (prevCircle) {
									prevCircle->merge(&c);
								} else {
									prevCircle = &c;
								}
							}
						}
					}
				}
			}

			// Find the best ball.
			AntiFlickerCircle *bestCircle = 0;
			for (std::list<AntiFlickerCircle *>::iterator i = antiFlickerCircles.begin(), iEnd = antiFlickerCircles.end(); i != iEnd; ++i) {
				assert(*i);
				if (!bestCircle || (*i)->popularity() > bestCircle->popularity()) {
					bestCircle = *i;
				}
			}

			// Move the circle to the ball position.
			if (bestCircle) {
				if (bestCircle->bestBall) {
					const SSL_DetectionBall &b = *bestCircle->bestBall;
					const Vector2 pos(b.x(), -b.y());
					bestCircle->centre() = pos;
				}
			}

			// Push up results.
			if (bestCircle) {
				World::get().ball().position(bestCircle->centre());
			}
		}

		// Check whether any balls are visible.
		{
			bool any = false;
			for (unsigned int i = 0; i < sizeof(detections) / sizeof(*detections); i++)
				any = any || !!detections[i].balls_size();
			World::get().isBallVisible(any);
		}

		// Handle the robots.
		const struct ColourSet {
			char colour;
			const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &(SSL_DetectionFrame::*data)() const;
		} colourSets[2] = {
			{'B', &SSL_DetectionFrame::robots_blue},
			{'Y', &SSL_DetectionFrame::robots_yellow}
		};
		{
			const SSL_DetectionRobot *bestByID[2 * Team::SIZE] = {};
			std::vector<const SSL_DetectionRobot *> unidentified[2];
			for (unsigned int clr = 0; clr < 2; clr++) {
				for (unsigned int cam = 0; cam < 2; cam++) {
					const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &data = (detections[cam].*colourSets[clr].data)();
					for (int i = 0; i < data.size(); i++) {
						const SSL_DetectionRobot &bot = data.Get(i);
						if (bot.has_robot_id()) {
							unsigned int irid = bot.robot_id();
							std::ostringstream oss;
							oss << colourSets[clr].colour << irid;
							const std::string &key = oss.str();
							if (Config::instance().hasKey("IR2AI", key)) {
								unsigned int aiid = Config::instance().getInteger<unsigned int>("IR2AI", key, 10);
								if (!bestByID[aiid] || bestByID[aiid]->confidence() < bot.confidence())
									bestByID[aiid] = &bot;
							}
						} else {
							unidentified[clr].push_back(&bot);
						}
					}
				}
			}

			// Now:
			// bestByID has the highest-confidence pattern for each AI ID where patterns are provided, or null if none.
			// unidentified has all the detections that don't have known patterns.
			//
			// The bots whose positions we know can be updated immediately. The others go into a bipartite matching.
			std::vector<unsigned int> unusedAIIDs[2];
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++)
				if (bestByID[i]) {
					PPlayer plr = World::get().player(i);
					plr->position(Vector2(bestByID[i]->x(), -bestByID[i]->y()));
					if (bestByID[i]->has_orientation()) {
						plr->orientation(bestByID[i]->orientation() / M_PI * 180.0);
					} else {
						plr->orientation(std::numeric_limits<double>::quiet_NaN());
					}
					plr->lastSeen().assign_current_time();
				} else {
					unusedAIIDs[i / Team::SIZE].push_back(i % Team::SIZE);
				}

			// Do bipartite matchings.
			for (unsigned int clr = 0; clr < 2; clr++) {
				if (!unidentified[clr].empty()) {
					std::ostringstream oss;
					oss << colourSets[clr].colour << 'H';
					const std::string &key = oss.str();
					unsigned int team = Config::instance().getInteger<unsigned int>("IR2AI", key, 10);
					Hungarian hung(std::max(unusedAIIDs[team].size(), unidentified[clr].size()));
					for (unsigned int x = 0; x < hung.size(); x++) {
						for (unsigned int y = 0; y < hung.size(); y++) {
							if (x < unidentified[clr].size() && y < unusedAIIDs[team].size()) {
								Vector2 oldPos = World::get().team(team).player(unusedAIIDs[team][y])->position();
								Vector2 newPos = Vector2(unidentified[clr][x]->x(), -unidentified[clr][x]->y());
								hung.weight(x, y) = World::get().field().infinity() - (newPos - oldPos).length();
							}
						}
					}
					hung.execute();
					for (unsigned int x = 0; x < unidentified[clr].size(); x++) {
						unsigned int y = hung.matchX(x);
						if (y < unusedAIIDs[team].size() && unusedAIIDs[team][y] < Team::SIZE) {
							const SSL_DetectionRobot &bot = *unidentified[clr][x];
							PPlayer plr = World::get().team(team).player(unusedAIIDs[team][y]);
							plr->position(Vector2(bot.x(), -bot.y()));
							if (bot.has_orientation()) {
								plr->orientation(bot.orientation() / M_PI * 180.0);
							} else {
								plr->orientation(std::numeric_limits<double>::quiet_NaN());
							}
							plr->lastSeen().assign_current_time();
						}
					}
				}
			}
		}

		const Vector2 &ballPos = World::get().ball().position();
		std::vector<PPlayer> possessors;
		for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
			PPlayer pl = World::get().player(i);
			Vector2 playerPoint = pl->position();
			if (pl->hasDefiniteOrientation()) {
				playerPoint += OFFSET_FROM_ROBOT_TO_BALL * Vector2(pl->orientation());
			}
			if ((ballPos - playerPoint).length() < 27) { // was 27
				possessors.push_back(pl);
			}
		}
		if (!possessors.empty()) {
			PPlayer pl = possessors[0];
			pl->hasBall(true);
		} else {
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++)
				World::get().player(i)->hasBall(false);
		}

		if (!World::get().isBallVisible()) {
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
				PPlayer pl = World::get().player(i);
				if (pl->hasBall()) {
					Vector2 ballPos = pl->position();
					if (pl->hasDefiniteOrientation()) {
						ballPos += OFFSET_FROM_ROBOT_TO_BALL * Vector2(pl->orientation());
					}
					World::get().ball().position(ballPos);
					break;
				}
			}
		}
	}

	return true;
}

bool ImageRecognition::onTimer() {
	const double inf = World::get().field().infinity();
	const Vector2 infv(inf, inf);
	Glib::TimeVal now;
	now.assign_current_time();
	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		Glib::TimeVal diff = now;
		diff -= World::get().player(i)->lastSeen();
		if (diff.as_double() > 1) {
			World::get().player(i)->position(infv);
		}
	}
	return true;
}

