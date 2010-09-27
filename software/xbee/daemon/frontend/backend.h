#ifndef XBEE_DAEMON_FRONTEND_BACKEND_H
#define XBEE_DAEMON_FRONTEND_BACKEND_H

#include "util/noncopyable.h"
#include <cstddef>
#include <stdint.h>
#include <vector>
#include <sigc++/sigc++.h>
#include <sys/uio.h>

/**
 * A particular back-end that wishes to handle all packets transmitted and produce received packets must subclass this class.
 * It must then pass said subclass into the constructor of the XBeeDaemon class.
 */
class BackEnd : public NonCopyable {
	public:
		/**
		 * Invoked when a packet is received.
		 * The subclass is expected to trigger this signal in order to inject a received packet into the system.
		 */
		sigc::signal<void, const std::vector<uint8_t> &> signal_received;

		/**
		 * Creates a new BackEnd object.
		 */
		BackEnd() {
		}

		/**
		 * Destroys a BackEnd object.
		 */
		virtual ~BackEnd() {
		}

		/**
		 * Sends a packet.
		 * The subclass is expected to implement this class to send the given packet.
		 *
		 * \param[in] iov a pointer to an array of iovecs to gather to find the bytes to send.
		 *
		 * \param[in] iovcnt the number of iovecs in the \p iov array.
		 */
		virtual void send(const iovec *iov, std::size_t iovcnt) = 0;
};

#endif

