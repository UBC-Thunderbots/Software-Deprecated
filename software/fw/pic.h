#ifndef FIRMWARE_PIC_H
#define FIRMWARE_PIC_H

#include "fw/bootproto.h"
#include "fw/watchable_operation.h"
#include "util/ihex.h"
#include "xbee/client/raw.h"

//
// An operation to upload data to be burned into the PIC.
//
class PICUpload : public WatchableOperation, public sigc::trackable {
	public:
		//
		// Constructs an uploader.
		//
		PICUpload(RefPtr<XBeeRawBot> bot, const IntelHex &data);

		//
		// Starts the upload process.
		//
		void start();

		//
		// The number of bytes in a page.
		//
		static const unsigned int PAGE_BYTES = 64;

	private:
		const RefPtr<XBeeRawBot> bot;
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

