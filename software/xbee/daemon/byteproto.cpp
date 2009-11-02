#include "xbee/daemon/byteproto.h"
#include <algorithm>

xbee_byte_stream::xbee_byte_stream() : received_escape(false) {
	port.signal_received().connect(sigc::mem_fun(*this, &xbee_byte_stream::bytes_received));
}

void xbee_byte_stream::send_sop() {
	port.send(0x7E);
}

void xbee_byte_stream::send(uint8_t ch) {
	if (ch == 0x7E || ch == 0x7D || ch == 0x11 || ch == 0x13) {
		const uint8_t buf[2] = {0x7D, ch ^ 0x20};
		port.send(buf, 2);
	} else {
		port.send(ch);
	}
}

void xbee_byte_stream::send(const void *payload, std::size_t length) {
	const uint8_t *dptr = static_cast<const uint8_t *>(payload);
	static const uint8_t SPECIAL_CHARS[] = {0x7E, 0x7D, 0x11, 0x13};

	while (length) {
		std::size_t index = std::find_first_of(dptr, dptr + length, SPECIAL_CHARS, SPECIAL_CHARS + sizeof(SPECIAL_CHARS) / sizeof(*SPECIAL_CHARS)) - dptr;
		if (index > 0) {
			port.send(dptr, index);
			dptr += index;
			length -= index;
		} else {
			send(dptr[0]);
			dptr++;
			length--;
		}
	}
}

void xbee_byte_stream::bytes_received(const void *data, std::size_t len) {
	const uint8_t *dptr = static_cast<const uint8_t *>(data);

	while (len) {
		uint8_t ch = *dptr++;
		len--;
		if (ch == 0x7E) {
			received_escape = false;
			sig_sop_received.emit();
		} else if (ch == 0x7D) {
			received_escape = true;
		} else {
			if (received_escape) {
				ch ^= 0x20;
				received_escape = false;
			}
			sig_byte_received.emit(ch);
		}
	}
}

