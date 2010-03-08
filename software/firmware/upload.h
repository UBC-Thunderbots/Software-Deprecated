#ifndef FIRMWARE_UPLOAD_H
#define FIRMWARE_UPLOAD_H

#include "firmware/bootproto.h"
#include "firmware/scheduler.h"
#include "firmware/watchable_operation.h"
#include "util/ihex.h"

//
// An in-progress firmware upgrade operation.
//
class upload : public watchable_operation, public sigc::trackable {
	public:
		//
		// Constructs an uploader object.
		//
		upload(xbee &modem, uint64_t bot, const intel_hex &data);

		//
		// Starts the upload process.
		//
		void start();

		//
		// Returns the number of CRC errors so far.
		//
		unsigned int crc_failure_count() const {
			return sched.crc_failure_count();
		}

	private:
		bootproto proto;
		upload_scheduler sched;
		upload_irp irp;

		void bootloader_entered();
		void ident_received(const void *);
		void send_next_irp();
		void irp_done(const void *);
		void submit_fpga_write_page();
		void submit_fpga_crc_chunk();
		void fpga_crc_chunk_done(const void *);
		void submit_fpga_erase_sector();
};

#endif

