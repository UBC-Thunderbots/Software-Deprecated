#define DEBUG 0
#include "util/dprint.h"
#include "util/xbee.h"
#include "xbee/client/packet.h"
#include "xbee/shared/packettypes.h"
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>

void transmit64_packet::transmit(const file_descriptor &sock, uint8_t frame) const {
	xbeepacket::TRANSMIT64_HDR hdr;
	hdr.apiid = xbeepacket::TRANSMIT64_APIID;
	hdr.frame = frame;
	xbeeutil::address_to_bytes(dest, hdr.address);
	hdr.options = disable_ack ? xbeepacket::TRANSMIT_OPTION_DISABLE_ACK : 0;

	iovec iov[2];
	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = const_cast<uint8_t *>(&data[0]);
	iov[1].iov_len = data.size();

	msghdr mh;
	mh.msg_name = 0;
	mh.msg_namelen = 0;
	mh.msg_iov = iov;
	mh.msg_iovlen = 2;
	mh.msg_control = 0;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;

	DPRINT("Transmitting data packet with 64-bit address.");

	if (sendmsg(sock, &mh, MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(hdr) + data.size())) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

template<std::size_t value_size>
void remote_at_packet<value_size>::transmit(const file_descriptor &sock, uint8_t frame) const {
	xbeepacket::REMOTE_AT_REQUEST<value_size> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = frame;
	xbeeutil::address_to_bytes(dest, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	std::copy(command, command + 2, packet.command);
	std::copy(value, value + value_size, packet.value);

	DPRINT("Transmitting remote AT command packet.");

	if (send(sock, &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

void meta_claim_packet::transmit(const file_descriptor &sock, uint8_t frame) const {
	assert(!frame);

	xbeepacket::META_CLAIM packet;
	packet.hdr.apiid = xbeepacket::META_APIID;
	packet.hdr.metatype = xbeepacket::CLAIM_METATYPE;
	packet.address = address;
	packet.drive_mode = drive_mode ? 1 : 0;

	DPRINT("Transmitting meta claim packet.");

	if (send(sock, &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

void meta_release_packet::transmit(const file_descriptor &sock, uint8_t frame) const {
	assert(!frame);

	xbeepacket::META_RELEASE packet;
	packet.hdr.apiid = xbeepacket::META_APIID;
	packet.hdr.metatype = xbeepacket::RELEASE_METATYPE;
	packet.address = address;

	DPRINT("Transmitting meta release packet.");

	if (send(sock, &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

