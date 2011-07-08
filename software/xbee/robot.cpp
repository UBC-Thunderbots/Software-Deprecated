#include "xbee/robot.h"
#include "xbee/dongle.h"
#include "xbee/kickpacket.h"
#include "xbee/testmodepacket.h"
#include <cassert>
#include <cstring>

#warning Doxygen

namespace {
	enum TBotsFirmwareRequest {
		FIRMWARE_REQUEST_CHIP_ERASE = 0,
		FIRMWARE_REQUEST_FILL_PAGE_BUFFER = 1,
		FIRMWARE_REQUEST_PAGE_PROGRAM = 2,
		FIRMWARE_REQUEST_CRC_BLOCK = 3,
		FIRMWARE_REQUEST_READ_PARAMS = 4,
		FIRMWARE_REQUEST_SET_PARAMS = 5,
		FIRMWARE_REQUEST_COMMIT_PARAMS = 7,
		FIRMWARE_REQUEST_REBOOT = 8,
		FIRMWARE_REQUEST_READ_BUILD_SIGNATURES = 9,
	};

	XBeeRobot::OperationalParameters::FlashContents flash_contents_of_int(uint8_t u8) {
		XBeeRobot::OperationalParameters::FlashContents fc = static_cast<XBeeRobot::OperationalParameters::FlashContents>(u8);
		switch (fc) {
			case XBeeRobot::OperationalParameters::FlashContents::FPGA:
			case XBeeRobot::OperationalParameters::FlashContents::PIC:
			case XBeeRobot::OperationalParameters::FlashContents::NONE:
				return fc;
		}
		throw std::runtime_error("Invalid flash contents byte");
	}

	class FirmwareSPIChipEraseOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareSPIChipEraseOperation(dongle, robot));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;

			FirmwareSPIChipEraseOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				const uint8_t data[] = { static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_CHIP_ERASE };
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIChipEraseOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSPIChipEraseOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_CHIP_ERASE) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareSPIFillPageBufferOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, unsigned int offset, const void *data, std::size_t length) {
				Ptr p(new FirmwareSPIFillPageBufferOperation(dongle, robot, offset, data, length));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			Ptr self_ref;

			FirmwareSPIFillPageBufferOperation(XBeeDongle &dongle, unsigned int robot, unsigned int offset, const void *data, std::size_t length) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t buffer[length + 3];
				assert(sizeof(buffer) <= 64);
				assert(offset + length <= 256);
				buffer[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				buffer[1] = FIRMWARE_REQUEST_FILL_PAGE_BUFFER;
				buffer[2] = static_cast<uint8_t>(offset);
				std::memcpy(buffer + 3, data, length);
				dongle.send_message(buffer, sizeof(buffer))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIFillPageBufferOperation::on_send_message_done));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					failed_operation = op;
				}
				Ptr pthis(this);
				self_ref.reset();
				signal_done.emit(pthis);
			}
	};

	class FirmwareSPIPageProgramOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, unsigned int page, uint16_t crc) {
				Ptr p(new FirmwareSPIPageProgramOperation(dongle, robot, page, crc));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;

			FirmwareSPIPageProgramOperation(XBeeDongle &dongle, unsigned int robot, unsigned int page, uint16_t crc) : robot(robot), self_ref(this) {
				uint8_t buffer[6];
				buffer[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				buffer[1] = FIRMWARE_REQUEST_PAGE_PROGRAM;
				buffer[2] = static_cast<uint8_t>(page);
				buffer[3] = static_cast<uint8_t>(page >> 8);
				buffer[4] = static_cast<uint8_t>(crc);
				buffer[5] = static_cast<uint8_t>(crc >> 8);
				send_connection = dongle.send_message(buffer, sizeof(buffer))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIPageProgramOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSPIPageProgramOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_PAGE_PROGRAM) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareSPIBlockCRCOperation : public AsyncOperation<uint16_t>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, unsigned int address, std::size_t length) {
				Ptr p(new FirmwareSPIBlockCRCOperation(dongle, robot, address, length));
				return p;
			}

			uint16_t result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
				return crc;
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			const unsigned int address;
			const std::size_t length;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			uint16_t crc;

			FirmwareSPIBlockCRCOperation(XBeeDongle &dongle, unsigned int robot, unsigned int address, std::size_t length) : dongle(dongle), robot(robot), address(address), length(length), self_ref(this) {
				assert(1 <= length && length <= 65536);
				uint8_t data[7];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_CRC_BLOCK;
				data[2] = static_cast<uint8_t>(address);
				data[3] = static_cast<uint8_t>(address >> 8);
				data[4] = static_cast<uint8_t>(address >> 16);
				data[5] = static_cast<uint8_t>(length);
				data[6] = static_cast<uint8_t>(length >> 8);
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIBlockCRCOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSPIBlockCRCOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 8 && pch[0] == FIRMWARE_REQUEST_CRC_BLOCK && static_cast<uint32_t>(pch[1] | (pch[2] << 8) | (pch[3] << 16)) == address && static_cast<uint16_t>(pch[4] | (pch[5] << 8)) == (length & 0xFFFF)) {
						crc = static_cast<uint16_t>(pch[6] | (pch[7] << 8));
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareReadOperationalParametersOperation : public AsyncOperation<XBeeRobot::OperationalParameters>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareReadOperationalParametersOperation(dongle, robot));
				return p;
			}

			XBeeRobot::OperationalParameters result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
				return params;
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareReadOperationalParametersOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_READ_PARAMS;
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareReadOperationalParametersOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareReadOperationalParametersOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 7 && pch[0] == FIRMWARE_REQUEST_READ_PARAMS) {
#warning sanity checks
						params.flash_contents = flash_contents_of_int(pch[1]);
						params.xbee_channels[0] = pch[2];
						params.xbee_channels[1] = pch[3];
						params.robot_number = pch[4];
						params.dribble_power = pch[5];
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareSetOperationalParametersOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, const XBeeRobot::OperationalParameters &params) {
				Ptr p(new FirmwareSetOperationalParametersOperation(dongle, robot, params));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareSetOperationalParametersOperation(XBeeDongle &dongle, unsigned int robot, const XBeeRobot::OperationalParameters &params) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[8];
#warning sanity checks
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_SET_PARAMS;
				data[2] = static_cast<uint8_t>(params.flash_contents);
				data[3] = params.xbee_channels[0];
				data[4] = params.xbee_channels[1];
				data[5] = params.robot_number;
				data[6] = params.dribble_power;
				data[7] = 0;
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareSetOperationalParametersOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSetOperationalParametersOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 1 && pch[0] == FIRMWARE_REQUEST_SET_PARAMS) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareCommitOperationalParametersOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareCommitOperationalParametersOperation(dongle, robot));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareCommitOperationalParametersOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_COMMIT_PARAMS;
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareCommitOperationalParametersOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareCommitOperationalParametersOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 1 && pch[0] == FIRMWARE_REQUEST_COMMIT_PARAMS) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareRebootOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareRebootOperation(dongle, robot));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareRebootOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_REBOOT;
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareRebootOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareRebootOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 1 && pch[0] == FIRMWARE_REQUEST_REBOOT) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareReadBuildSignaturesOperation : public AsyncOperation<XBeeRobot::BuildSignatures>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareReadBuildSignaturesOperation(dongle, robot));
				return p;
			}

			XBeeRobot::BuildSignatures result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
				return sigs;
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::BuildSignatures sigs;

			FirmwareReadBuildSignaturesOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_READ_BUILD_SIGNATURES;
				send_connection = dongle.send_message(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareReadBuildSignaturesOperation::on_send_message_done));
				receive_connection = dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareReadBuildSignaturesOperation::on_message_received));
			}

			void on_send_message_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 5 && pch[0] == FIRMWARE_REQUEST_READ_BUILD_SIGNATURES) {
#warning sanity checks
						sigs.firmware_signature = static_cast<uint16_t>(pch[1] | (pch[2] << 8));
						sigs.flash_signature = static_cast<uint16_t>(pch[3] | (pch[4] << 8));
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	void discard_result(AsyncOperation<void>::Ptr op) {
		op->result();
	}

	void encode_kick_parameters(unsigned int &pulse_width1, unsigned int &pulse_width2, unsigned int &slice_width, bool &ignore_slice1, bool &ignore_slice2, int offset) {
		assert(pulse_width1 <= 4064);
		assert(pulse_width2 <= 4064);
		assert(-4064 <= offset && offset <= 4064);

		/* The parameters pulse_width1, pulse_width2, and offset come as input.
		 * These are numbers in microseconds; the pulse_widths are the actual widths and offset is the time difference between their leading edges.
		 *
		 * For transmission to the robot, however, a number of changes need to be made.
		 *
		 * First, the FPGA bitstream doesn't use pulse widths and a leading edge offset.
		 * For each solenoid, it accepts the time from the start of the kick operation to the falling edge of the pulse.
		 * It also accepts a “slice width”, which is the time from the start of the kick operation to the rising edge of some of the pulses.
		 * Finally, it accepts two flags indicating whether each pulse's rising edge is at the start of the operation or after the slice width.
		 *
		 * Second, times sent to the robot over the radio are represented in 32µs units rather than in individual microseconds. */
		if (offset < 0) {
			slice_width = -offset;
			pulse_width2 += slice_width;
			assert(pulse_width2 <= 4064);
			ignore_slice1 = true;
			ignore_slice2 = false;
		} else if (offset > 0) {
			slice_width = offset;
			pulse_width1 += slice_width;
			assert(pulse_width1 <= 4064);
			ignore_slice1 = false;
			ignore_slice2 = true;
		} else {
			slice_width = 0;
			ignore_slice1 = true;
			ignore_slice2 = true;
		}
		pulse_width1 /= 32;
		pulse_width2 /= 32;
		slice_width /= 32;
		assert(pulse_width1 <= 127);
		assert(pulse_width2 <= 127);
		assert(slice_width <= 127);
	}
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_spi_chip_erase() {
	return FirmwareSPIChipEraseOperation::create(dongle, index);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_spi_fill_page_buffer(unsigned int offset, const void *data, std::size_t length) {
	return FirmwareSPIFillPageBufferOperation::create(dongle, index, offset, data, length);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_spi_page_program(unsigned int page, uint16_t crc) {
	return FirmwareSPIPageProgramOperation::create(dongle, index, page, crc);
}

AsyncOperation<uint16_t>::Ptr XBeeRobot::firmware_spi_block_crc(unsigned int address, std::size_t length) {
	return FirmwareSPIBlockCRCOperation::create(dongle, index, address, length);
}

AsyncOperation<XBeeRobot::OperationalParameters>::Ptr XBeeRobot::firmware_read_operational_parameters() {
	return FirmwareReadOperationalParametersOperation::create(dongle, index);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_set_operational_parameters(const OperationalParameters &params) {
	return FirmwareSetOperationalParametersOperation::create(dongle, index, params);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_commit_operational_parameters() {
	return FirmwareCommitOperationalParametersOperation::create(dongle, index);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_reboot() {
	return FirmwareRebootOperation::create(dongle, index);
}

AsyncOperation<XBeeRobot::BuildSignatures>::Ptr XBeeRobot::firmware_read_build_signatures() {
	return FirmwareReadBuildSignaturesOperation::create(dongle, index);
}

void XBeeRobot::drive(const int(&wheels)[4], bool controlled) {
	static int16_t(XBeePackets::Drive::*const ELTS[4]) = {
		&XBeePackets::Drive::wheel1,
		&XBeePackets::Drive::wheel2,
		&XBeePackets::Drive::wheel3,
		&XBeePackets::Drive::wheel4,
	};
	static_assert(G_N_ELEMENTS(wheels) == G_N_ELEMENTS(ELTS), "Number of wheels is wrong!");

	drive_block.enable_wheels = true;
	drive_block.enable_controllers = controlled;
	for (std::size_t i = 0; i < G_N_ELEMENTS(wheels); ++i) {
		assert(-1023 <= wheels[i] && wheels[i] <= 1023);
		drive_block.*ELTS[i] = static_cast<int16_t>(wheels[i]);
	}

	flush_drive();
}

void XBeeRobot::drive_scram() {
	drive_block.enable_wheels = false;
	flush_drive();
}

void XBeeRobot::dribble(bool active) {
	drive_block.enable_dribbler = active;
	flush_drive();
}

void XBeeRobot::enable_charger(bool active) {
	drive_block.enable_charger = active;
	flush_drive();
}

void XBeeRobot::autokick(unsigned int pulse_width1, unsigned int pulse_width2, int offset) {
	assert(pulse_width1 <= 4064);
	assert(pulse_width2 <= 4064);
	assert(-4064 <= offset && offset <= 4064);

	drive_block.enable_autokick = pulse_width1 || pulse_width2;
	drive_block.autokick_offset_sign = offset < 0;
	drive_block.autokick_width1 = static_cast<uint8_t>(pulse_width1 / 32);
	drive_block.autokick_width2 = static_cast<uint8_t>(pulse_width2 / 32);
	drive_block.autokick_offset = static_cast<uint8_t>(std::abs(offset) / 32);

	flush_drive();
}

void XBeeRobot::kick(unsigned int pulse_width1, unsigned int pulse_width2, int offset) {
	assert(pulse_width1 <= 4064);
	assert(pulse_width2 <= 4064);
	assert(-4064 <= offset && offset <= 4064);

	XBeePackets::Kick packet;
	packet.width1 = static_cast<uint8_t>(pulse_width1 / 32);
	packet.width2 = static_cast<uint8_t>(pulse_width2 / 32);
	packet.offset_sign = offset < 0;
	packet.offset = static_cast<uint8_t>(std::abs(offset) / 32);

	uint8_t buffer[1 + packet.BUFFER_SIZE];
	buffer[0] = static_cast<uint8_t>((index << 4) | XBeeDongle::PIPE_KICK);
	packet.encode(buffer + 1);

	dongle.send_message(buffer, sizeof(buffer))->signal_done.connect(&discard_result);
}

void XBeeRobot::test_mode(unsigned int mode) {
	assert((mode & 0xFFFF) == mode);

	XBeePackets::TestMode packet;
	packet.test_class = static_cast<uint8_t>(mode >> 8);
	packet.test_index = static_cast<uint8_t>(mode);

	uint8_t buffer[1 + packet.BUFFER_SIZE];
	buffer[0] = static_cast<uint8_t>((index << 4) | XBeeDongle::PIPE_TEST_MODE);
	packet.encode(buffer + 1);

	dongle.send_message(buffer, sizeof(buffer))->signal_done.connect(&discard_result);
}

XBeeRobot::Ptr XBeeRobot::create(XBeeDongle &dongle, unsigned int index) {
	Ptr p(new XBeeRobot(dongle, index));
	return p;
}

XBeeRobot::XBeeRobot(XBeeDongle &dongle, unsigned int index) : index(index), alive(false), has_feedback(false), ball_in_beam(false), capacitor_charged(false), battery_voltage(0), capacitor_voltage(0), dribbler_temperature(0), break_beam_reading(0), dongle(dongle), encoder_1_stuck_message(Glib::ustring::compose("Bot %1 encoder 1 not commutating", index), Annunciator::Message::TriggerMode::LEVEL), encoder_2_stuck_message(Glib::ustring::compose("Bot %1 encoder 2 not commutating", index), Annunciator::Message::TriggerMode::LEVEL), encoder_3_stuck_message(Glib::ustring::compose("Bot %1 encoder 3 not commutating", index), Annunciator::Message::TriggerMode::LEVEL), encoder_4_stuck_message(Glib::ustring::compose("Bot %1 encoder 4 not commutating", index), Annunciator::Message::TriggerMode::LEVEL), hall_stuck_message(Glib::ustring::compose("Bot %1 hall sensor stuck", index), Annunciator::Message::TriggerMode::LEVEL) {
}

void XBeeRobot::flush_drive(bool force) {
	if (force || drive_block != last_drive_block) {
		last_drive_block = drive_block;
		dongle.dirty_drive(index);
	}
}

void XBeeRobot::on_feedback(const uint8_t *data, std::size_t length) {
	assert(length == 9);
	has_feedback = !!(data[0] & 0x01);
	ball_in_beam = !!(data[0] & 0x02);
	capacitor_charged = !!(data[0] & 0x04);
	encoder_1_stuck_message.active(!!(data[0] & 0x08));
	encoder_2_stuck_message.active(!!(data[0] & 0x10));
	encoder_3_stuck_message.active(!!(data[0] & 0x20));
	encoder_4_stuck_message.active(!!(data[0] & 0x40));
	hall_stuck_message.active(!!(data[0] & 0x80));
	uint16_t ui16 = static_cast<uint16_t>(data[1] | (data[2] << 8));
	battery_voltage = (ui16 + 0.5) / 1024.0 * 3.3 / 330.0 * (1500.0 + 330.0);
	ui16 = static_cast<uint16_t>(data[3] | (data[4] << 8));
	capacitor_voltage = ui16 / 4096.0 * 3.3 / 2200.0 * (220000.0 + 2200.0);
	ui16 = static_cast<uint16_t>(data[5] | (data[6] << 8));
	dribbler_temperature = ui16 * 0.5735 - 205.9815;
	ui16 = static_cast<uint16_t>(data[7] | (data[8] << 8));
	break_beam_reading = ui16;
#warning deal with faults
}

void XBeeRobot::start_experiment(uint8_t control_code) {
	uint8_t buffer[2];
	buffer[0] = static_cast<uint8_t>((index << 4) | XBeeDongle::PIPE_EXPERIMENT_CONTROL);
	buffer[1] = control_code;

	dongle.send_message(buffer, sizeof(buffer))->signal_done.connect(&discard_result);
}

