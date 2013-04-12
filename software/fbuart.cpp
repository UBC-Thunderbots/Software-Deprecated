#include <cstdint>
#include <exception>
#include <iostream>
#include <glibmm/exception.h>
#include "util/libusb.h"

namespace {
	enum {
		CONTROL_REQUEST_READ_IO,
		CONTROL_REQUEST_WRITE_IO,
		CONTROL_REQUEST_JEDEC_ID,
		CONTROL_REQUEST_READ_STATUS,
		CONTROL_REQUEST_READ,
		CONTROL_REQUEST_WRITE,
		CONTROL_REQUEST_ERASE,
		CONTROL_REQUEST_GET_ERRORS,
	};

	int main_impl() {
		// Set up the device.
		USB::Context usb_context;
		USB::DeviceHandle usb_handle(usb_context, 0x0483, 0x497D);
		USB::ConfigurationSetter usb_config_setter(usb_handle, 4);
		USB::InterfaceClaimer usb_interface_claimer(usb_handle, 0);

		for (;;) {
			try {
				// Try receiving one transfer, which contains up to 256 bytes of received serial data.
				char buffer[256];
				std::size_t bytes_received = usb_handle.bulk_in(1, buffer, sizeof(buffer), 0);
				std::cout.write(buffer, bytes_received);
				std::cout.flush();
			} catch (const USB::TransferStallError &) {
				// An endpoint halt is not fatal; rather, it indicates that there is a reportable error which needs to be retrieved and displayed.
				try {
					// Extract and print the error.
					uint8_t errors;
					std::size_t bytes_received = usb_handle.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_GET_ERRORS, 0, 0, &errors, 1, 0);
					if (bytes_received != 1) {
						std::cerr << "\n\nProtocol error: improper number of bytes received for CONTROL_REQUEST_GET_ERRORS, expected 1, got " << bytes_received << "!\n";
						return 1;
					}
					if (!errors) {
						std::cerr << "\n\nProtocol error: CONTROL_REQUEST_GET_ERRORS returned zero!\n";
						return 1;
					}
					if (errors & 0xF0) {
						std::cerr << "\n\nProtocol error: CONTROL_REQUEST_GET_ERRORS returned value " << static_cast<unsigned int>(errors) << " with reserved bits set!\n";
						return 1;
					}
					if (errors & 0x08) {
						std::cerr << "\n\nSerial overrun error detected.\n\n";
					}
					if (errors & 0x04) {
						std::cerr << "\n\nUSB overrun error detected.\n\n";
					}
					if (errors & 0x01) {
						std::cerr << "\n\nFraming error detected on next byte.\n\n";
					}
					if (errors & 0x02) {
						std::cerr << "\n\nNoise detected on next byte.\n\n";
					}
				} catch (const USB::TransferStallError &) {
					// Catch this exception and print a more informative message; this should never happen.
					std::cerr << "\n\nProtocol error: bulk endpoint halted but CONTROL_REQUEST_GET_ERRORS returns protocol stall!\n";
					return 1;
				}

				// Now that errors have been retrieved and displayed, try to resume operating; however, this may fail due to more reportable errors accumulating, which is OK.
				try {
					usb_handle.clear_halt_in(1);
				} catch (const USB::TransferStallError &) {
				}
			}
		}
	}
}

int main() {
	try {
		return main_impl();
	} catch (const Glib::Exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

