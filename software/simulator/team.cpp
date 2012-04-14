#include "simulator/team.h"
#include "simulator/simulator.h"
#include "util/codec.h"
#include "util/exception.h"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>
#include <unistd.h>
#include <utility>

namespace {
	bool pattern_equals(const Simulator::Team::PlayerInfo &pi, unsigned int pattern) {
		return pi.pattern == pattern;
	}
}

Simulator::Team::Team(Simulator &sim, const Team *other, bool invert) : sim(sim), other(other), invert(invert), ready_(true) {
}

bool Simulator::Team::has_connection() const {
	return !!connection;
}

void Simulator::Team::add_connection(Connection::Ptr &&conn) {
	assert(!connection);
	assert(conn);
	conn->signal_closed().connect(sigc::mem_fun(this, &Team::close_connection));
	conn->signal_packet().connect(sigc::mem_fun(this, &Team::on_packet));
	connection = std::move(conn);
	ready_ = true;
	Glib::signal_idle().connect_once(sigc::mem_fun(this, &Team::send_speed_mode));
	Glib::signal_idle().connect_once(signal_ready().make_slot());
}

bool Simulator::Team::ready() const {
	return ready_;
}

sigc::signal<void> &Simulator::Team::signal_ready() const {
	return signal_ready_;
}

void Simulator::Team::send_tick(const timespec &ts) {
	// Actually remove any robots we were asked to remove in the past time period.
	for (std::vector<unsigned int>::const_iterator i = to_remove.begin(), iend = to_remove.end(); i != iend; ++i) {
		std::vector<PlayerInfo>::iterator j = std::find_if(players.begin(), players.end(), [&i](PlayerInfo p) { return p.pattern == *i; });
		if (j == players.end()) {
			std::cout << "AI asked to remove nonexistent robot pattern " << *i << '\n';
			close_connection();
			return;
		}
		Player *plr = j->player;
		players.erase(j);
		sim.engine.remove_player(plr);
	}
	to_remove.clear();

	// Actually add any robots we were asked to add in the past time period.
	for (std::vector<unsigned int>::const_iterator i = to_add.begin(), iend = to_add.end(); i != iend; ++i) {
		if (std::find_if(players.begin(), players.end(), [&i](PlayerInfo p) { return p.pattern == *i; }) != players.end()) {
			std::cout << "AI asked to add robot pattern " << *i << " twice\n";
			close_connection();
			return;
		}
		PlayerInfo pi = { player: sim.engine.add_player(), pattern: *i, };
		players.push_back(pi);
	}
	to_add.clear();

	if (connection) {
		// Send the state of the world over the socket.
		Proto::S2APacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = Proto::S2APacketType::TICK;
		sim.encode_ball_state(packet.world_state.ball, invert);
		for (std::size_t i = 0; i < G_N_ELEMENTS(packet.world_state.friendly); ++i) {
			if (i < players.size()) {
				packet.world_state.friendly[i].robot_info.pattern = players[i].pattern;
				packet.world_state.friendly[i].robot_info.x = players[i].player->position().x * (invert ? -1.0 : 1.0);
				packet.world_state.friendly[i].robot_info.y = players[i].player->position().y * (invert ? -1.0 : 1.0);
				packet.world_state.friendly[i].robot_info.orientation = (players[i].player->orientation() + (invert ? Angle::HALF : Angle::ZERO)).to_radians();
				packet.world_state.friendly[i].has_ball = players[i].player->has_ball();
			} else {
				packet.world_state.friendly[i].robot_info.pattern = std::numeric_limits<unsigned int>::max();
			}
		}
		for (std::size_t i = 0; i < G_N_ELEMENTS(packet.world_state.enemy); ++i) {
			if (i < other->players.size()) {
				packet.world_state.enemy[i].pattern = other->players[i].pattern;
				packet.world_state.enemy[i].x = other->players[i].player->position().x * (invert ? -1.0 : 1.0);
				packet.world_state.enemy[i].y = other->players[i].player->position().y * (invert ? -1.0 : 1.0);
				packet.world_state.enemy[i].orientation = (other->players[i].player->orientation() + (invert ? Angle::HALF : Angle::ZERO)).to_radians();
			} else {
				packet.world_state.enemy[i].pattern = std::numeric_limits<unsigned int>::max();
			}
		}
		packet.world_state.stamp = ts;
		packet.world_state.friendly_score = 0;
		packet.world_state.enemy_score = 0;
		connection->send(packet);

		// Having sent the tick packet, we are no longer ready and must wait for an A2S_PACKET_PLAYERS.
		ready_ = false;
	}
}

void Simulator::Team::send_speed_mode() {
	if (connection) {
		Proto::S2APacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = Proto::S2APacketType::SPEED_MODE;
		packet.speed_mode = sim.speed_mode();
		connection->send(packet);
	}
}

void Simulator::Team::load_state(const FileDescriptor &fd) {
	// Any prior requests to add or remove players are no longer relevant.
	to_add.clear();
	to_remove.clear();

	// Remove all robots from the team.
	std::for_each(players.begin(), players.end(), [&sim](PlayerInfo p) { sim.engine.remove_player(p.player); });
	players.clear();

	// Read the size of the team.
	uint8_t size;
	{
		ssize_t rc = read(fd.fd(), &size, sizeof(size));
		if (rc < 0) {
			throw SystemError("read", errno);
		} else if (!rc) {
			throw std::runtime_error("Premature EOF in state file");
		} else if (size > Proto::MAX_PLAYERS_PER_TEAM) {
			throw std::runtime_error("Corrupt state file");
		}
	}

	// Create the robots.
	while (size--) {
		// Read lid pattern.
		uint32_t pattern;
		{
			uint8_t buffer[sizeof(pattern)];
			ssize_t rc = read(fd.fd(), buffer, sizeof(buffer));
			if (rc < 0) {
				throw SystemError("read", errno);
			} else if (rc != sizeof(buffer)) {
				throw std::runtime_error("Premature EOF in state file");
			}
			pattern = decode_u32(buffer);
		}

		// Create information structure.
		PlayerInfo pi = { player: sim.engine.add_player(), pattern: static_cast<unsigned int>(pattern), };

		// Reload player state for engine.
		pi.player->load_state(fd);

		// Done!
		players.push_back(pi);
	}
}

void Simulator::Team::save_state(const FileDescriptor &fd) const {
	// Write the size of the team.
	uint8_t size = static_cast<uint8_t>(players.size());
	if (write(fd.fd(), &size, sizeof(size)) != sizeof(size)) {
		throw SystemError("write", errno);
	}

	// Write the robots.
	for (std::vector<PlayerInfo>::const_iterator i = players.begin(), iend = players.end(); i != iend; ++i) {
		// Write the lid pattern.
		uint8_t buffer[sizeof(uint32_t)];
		encode_u32(buffer, static_cast<uint32_t>(i->pattern));
		if (write(fd.fd(), buffer, sizeof(buffer)) != sizeof(buffer)) {
			throw SystemError("write", errno);
		}

		// Save engine data.
		i->player->save_state(fd);
	}
}

void Simulator::Team::close_connection() {
	// Drop the socket.
	connection.reset();

	// If no AI is connected, we should not prevent the other team from making forward progress with simulation.
	ready_ = true;
	Glib::signal_idle().connect_once(signal_ready().make_slot());

	// Stop all our robots from kicking, chipping, or moving.
	for (auto i = players.begin(), iend = players.end(); i != iend; ++i) {
		i->player->orders.kick = false;
		i->player->orders.chip = false;
		i->player->orders.chick_power = 0.0;
		std::fill(&i->player->orders.wheel_speeds[0], &i->player->orders.wheel_speeds[4], 0);
	}
}

void Simulator::Team::on_packet(const Proto::A2SPacket &packet, std::shared_ptr<FileDescriptor> ancillary_fd) {
	switch (packet.type) {
		case Proto::A2SPacketType::PLAYERS:
			// This should only be received if we're waiting for it after an S2A_PACKET_TICK.
			if (ready_) {
				std::cout << "AI out of phase, sent orders twice\n";
				close_connection();
				return;
			}

			// Stash the robots' orders into their Player objects for the engine to examine.
			for (std::size_t i = 0; i < G_N_ELEMENTS(packet.players); ++i) {
				if (packet.players[i].pattern != std::numeric_limits<unsigned int>::max()) {
					std::vector<PlayerInfo>::iterator j = std::find_if(players.begin(), players.end(), [&packet, i](PlayerInfo p) { return p.pattern == packet.players[i].pattern; });
					if (j == players.end()) {
						std::cout << "AI sent orders to nonexistent robot pattern " << packet.players[i].pattern << '\n';
						close_connection();
						return;
					}
					j->player->orders = packet.players[i];
				}
			}

			// Mark this team as ready for another tick.
			ready_ = true;
			signal_ready().emit();
			return;

		case Proto::A2SPacketType::ADD_PLAYER:
			if (packet.pattern >= Proto::MAX_PLAYERS_PER_TEAM) {
				std::cout << "AI asked to add invalid robot pattern " << packet.pattern << '\n';
				close_connection();
				return;
			}
			to_add.push_back(packet.pattern);
			return;

		case Proto::A2SPacketType::REMOVE_PLAYER:
			if (packet.pattern >= Proto::MAX_PLAYERS_PER_TEAM) {
				std::cout << "AI asked to remove invalid robot pattern " << packet.pattern << '\n';
				close_connection();
				return;
			}
			to_remove.push_back(packet.pattern);
			return;

		case Proto::A2SPacketType::SET_SPEED:
			sim.speed_mode(packet.speed_mode);
			return;

		case Proto::A2SPacketType::DRAG_BALL:
		{
			Point p(packet.drag.x, packet.drag.y);
			sim.engine.get_ball().position(invert ? -p : p);
		}
			return;

		case Proto::A2SPacketType::DRAG_PLAYER:
		{
			std::vector<PlayerInfo>::iterator i = std::find_if(players.begin(), players.end(), [&packet](PlayerInfo p) { return p.pattern == packet.drag.pattern; });
			if (i == players.end()) {
				std::cout << "AI dragged nonexistent robot pattern " << packet.drag.pattern << '\n';
				close_connection();
				return;
			}
			Point p(packet.drag.x, packet.drag.y);
			i->player->position(invert ? -p : p);
			return;
		}

		case Proto::A2SPacketType::LOAD_STATE:
		{
			if (!ancillary_fd) {
				std::cout << "AI sent A2S_PACKET_LOAD_STATE without ancillary file descriptor.\n";
				close_connection();
				return;
			}
			sim.queue_load_state(ancillary_fd);
			return;
		}

		case Proto::A2SPacketType::SAVE_STATE:
		{
			if (!ancillary_fd) {
				std::cout << "AI sent A2S_PACKET_SAVE_STATE without ancillary file descriptor.\n";
				close_connection();
				return;
			}
			sim.save_state(ancillary_fd);
			return;
		}
	}

	std::cout << "AI sent bad packet type\n";
	close_connection();
}

