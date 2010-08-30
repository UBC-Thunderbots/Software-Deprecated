#ifndef FIRMWARE_PIC_H
#define FIRMWARE_PIC_H

#include "fw/bootproto.h"
#include "fw/ihex.h"
#include "fw/watchable_operation.h"
#include "xbee/client/raw.h"

/**
 * An operation to upload data to be burned into the PIC.
 */
class PICUpload : public WatchableOperation, public sigc::trackable {
	public:
		/**
		 * Constructs a PICUpload.
		 *
		 * \param[in] bot the robot whose PIC firmware should be upgraded.
		 *
		 * \param[in] data the new firmware to upload.
		 */
		PICUpload(XBeeRawBot::Ptr bot, const IntelHex &data);

		/**
		 * Starts the upload process.
		 */
		void start();

		/**
		 * The number of bytes in a page.
		 */
		static const unsigned int PAGE_BYTES = 64;

	private:
		const XBeeRawBot::Ptr bot;
		const IntelHex &data;
		BootProto proto;
		unsigned int pages_written;

		void enter_bootloader_done();
		void ident_received(const void *);
		void fuses_received(const void *);
		void do_work();
		void page_written(const void *);
		void upgrade_enabled(const void *);
};

#endif

