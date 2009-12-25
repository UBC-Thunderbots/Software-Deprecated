#ifndef FIRMWARE_WATCHABLE_OPERATION_H
#define FIRMWARE_WATCHABLE_OPERATION_H

#include "util/noncopyable.h"
#include <glibmm.h>

//
// A generic operation whose progress can be observed.
//
class watchable_operation : public noncopyable {
	public:
		//
		// Destroys the object.
		//
		virtual ~watchable_operation() {
		}

		//
		// Starts the operation.
		//
		virtual void start() = 0;

		//
		// Fired whenever progress is made.
		//
		sigc::signal<void, double> &signal_progress() {
			return sig_progress;
		}

		//
		// Fired when the operation completes.
		//
		sigc::signal<void> &signal_finished() {
			return sig_finished;
		}

		//
		// Fired when an error occurs. No further activity should occur.
		//
		sigc::signal<void, const Glib::ustring &> &signal_error() {
			return sig_error;
		}

		//
		// Returns the textual status of the current operation stage.
		//
		const Glib::ustring &get_status() const {
			return status;
		}

	protected:
		//
		// The current status (should be set by subclasses).
		//
		Glib::ustring status;

	private:
		sigc::signal<void, double> sig_progress;
		sigc::signal<void> sig_finished;
		sigc::signal<void, const Glib::ustring &> sig_error;
};

#endif

