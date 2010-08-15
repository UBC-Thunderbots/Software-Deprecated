#ifndef XBEE_DAEMON_FRONTEND_ALREADY_RUNNING_H
#define XBEE_DAEMON_FRONTEND_ALREADY_RUNNING_H

#include <exception>

/**
 * This exception is thrown if an attempt is made to construct a XBeeDaemon object
 * while an arbiter is already running (as detected by the lock file being
 * locked).
 */
class AlreadyRunning : public std::exception {
	public:
		/**
		 * Returns a string message describing the situation.
		 *
		 * \return the message.
		 */
		const char *what() const throw() {
			return "An XBee arbiter is already running.";
		}
};

#endif

