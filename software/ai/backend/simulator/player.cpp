#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/backend.h"
#include <algorithm>
#include <cstring>

AI::BE::Simulator::Player::Ptr AI::BE::Simulator::Player::create(Backend &be, unsigned int pattern) {
	Ptr p(new Player(be, pattern));
	return p;
}

void AI::BE::Simulator::Player::pre_tick(const ::Simulator::Proto::S2APlayerInfo &state, const timespec &ts) {
	AI::BE::Player::pre_tick();
	AI::BE::Simulator::Robot::pre_tick(state.robot_info, ts);
	has_ball_ = state.has_ball;
	kick_ = false;
	autokick_fired_ = autokick_pre_fired_;
	autokick_pre_fired_ = false;
}

void AI::BE::Simulator::Player::encode_orders(::Simulator::Proto::A2SPlayerInfo &orders) {
	orders.pattern = pattern();
	orders.kick = kick_;
	orders.chip = false;
	orders.chick_power = chick_power_;
	std::copy(&wheel_speeds_[0], &wheel_speeds_[4], &orders.wheel_speeds[0]);
}

void AI::BE::Simulator::Player::mouse_pressed(Point p, unsigned int btn) {
	if (btn == 1 && (p - position()).len() < MAX_RADIUS) {
		disconnect_mouse();
		mouse_connections[0] = be.signal_mouse_released.connect(sigc::mem_fun(this, &Player::mouse_released));
		mouse_connections[1] = be.signal_mouse_exited.connect(sigc::mem_fun(this, &Player::mouse_exited));
		mouse_connections[2] = be.signal_mouse_moved.connect(sigc::mem_fun(this, &Player::mouse_moved));
	}
}

void AI::BE::Simulator::Player::mouse_released(Point, unsigned int btn) {
	if (btn == 1) {
		disconnect_mouse();
	}
}

void AI::BE::Simulator::Player::mouse_exited() {
	disconnect_mouse();
}

void AI::BE::Simulator::Player::mouse_moved(Point p) {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::DRAG_PLAYER;
	packet.drag.pattern = pattern();
	packet.drag.x = p.x;
	packet.drag.y = p.y;
	be.send_packet(packet);
}

Visualizable::Colour AI::BE::Simulator::Player::visualizer_colour() const {
	return Visualizable::Colour(0.0, 1.0, 0.0);
}

bool AI::BE::Simulator::Player::highlight() const {
	return mouse_connections[0].connected();
}

bool AI::BE::Simulator::Player::alive() const {
	return true;
}

bool AI::BE::Simulator::Player::has_ball() const {
	return has_ball_;
}

bool AI::BE::Simulator::Player::chicker_ready() const {
	return true;
}

bool AI::BE::Simulator::Player::kicker_directional() const {
	return false;
}

void AI::BE::Simulator::Player::kick_impl(double speed, Angle) {
	kick_ = true;
	chick_power_ = speed;
}

void AI::BE::Simulator::Player::autokick_impl(double speed, Angle angle) {
	kick_impl(speed, angle);
	autokick_pre_fired_ = true;
}

bool AI::BE::Simulator::Player::autokick_fired() const {
	return autokick_fired_;
}

bool AI::BE::Simulator::Player::has_destination() const {
	return true;
}

const std::pair<Point, Angle> &AI::BE::Simulator::Player::destination() const {
	return destination_;
}

Point AI::BE::Simulator::Player::target_velocity() const {
	return target_velocity_;
}

void AI::BE::Simulator::Player::path_impl(const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &p) {
	path_ = p;
}

bool AI::BE::Simulator::Player::has_path() const {
	return true;
}

const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &AI::BE::Simulator::Player::path() const {
	return path_;
}

void AI::BE::Simulator::Player::drive(const int(&w)[4]) {
	std::copy(&w[0], &w[4], &wheel_speeds_[0]);
}

const int(&AI::BE::Simulator::Player::wheel_speeds() const)[4] {
	return wheel_speeds_;
}

AI::BE::Simulator::Player::Player(Backend &be, unsigned int pattern) : AI::BE::Simulator::Robot(pattern), be(be), has_ball_(false), kick_(false), chick_power_(0.0), autokick_fired_(false), autokick_pre_fired_(false) {
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
	be.signal_mouse_pressed.connect(sigc::mem_fun(this, &Player::mouse_pressed));
}

AI::BE::Simulator::Player::~Player() = default;

void AI::BE::Simulator::Player::disconnect_mouse() {
	for (std::size_t i = 0; i < G_N_ELEMENTS(mouse_connections); ++i) {
		mouse_connections[i].disconnect();
	}
}

