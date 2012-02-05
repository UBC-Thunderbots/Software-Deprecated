#include "xbee/robot.h"
#include "xbee/dongle.h"
#include "xbee/kickpacket.h"
#include "xbee/testmodepacket.h"
#include <cassert>
#include <cstring>
#include <memory>

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

	void discard_result(AsyncOperation<void> &op) {
		op.result();
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

XBeeRobot::FirmwareSPIChipEraseOperation::FirmwareSPIChipEraseOperation(XBeeRobot &robot) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_CHIP_ERASE}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &FirmwareSPIChipEraseOperation::send_message_op_done));
}

void XBeeRobot::FirmwareSPIChipEraseOperation::result() const {
	send_message_op.result();
}

void XBeeRobot::FirmwareSPIChipEraseOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSPIChipEraseOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareSPIChipEraseOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_CHIP_ERASE) {
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::FirmwareSPIFillPageBufferOperation::FirmwareSPIFillPageBufferOperation(XBeeRobot &robot, unsigned int offset, const void *data, std::size_t length) : robot(robot), send_message_op(robot.dongle, prepare_buffer(offset, data, length), length + 3) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &FirmwareSPIFillPageBufferOperation::send_message_op_done));
}

void XBeeRobot::FirmwareSPIFillPageBufferOperation::result() const {
	send_message_op.result();
}

void XBeeRobot::FirmwareSPIFillPageBufferOperation::send_message_op_done(AsyncOperation<void> &) {
	signal_done.emit(*this);
}

uint8_t *XBeeRobot::FirmwareSPIFillPageBufferOperation::prepare_buffer(unsigned int offset, const void *data, std::size_t length) {
	assert(length + 3 <= sizeof(buffer));
	assert(offset + length <= 256);
	buffer[0] = static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
	buffer[1] = FIRMWARE_REQUEST_FILL_PAGE_BUFFER;
	buffer[2] = static_cast<uint8_t>(offset);
	std::memcpy(buffer + 3, data, length);
	return buffer;
}

XBeeRobot::FirmwareSPIPageProgramOperation::FirmwareSPIPageProgramOperation(XBeeRobot &robot, unsigned int page, uint16_t crc) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_PAGE_PROGRAM, static_cast<uint8_t>(page), static_cast<uint8_t>(page >> 8), static_cast<uint8_t>(crc), static_cast<uint8_t>(crc >> 8)}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::FirmwareSPIPageProgramOperation::send_message_op_done));
}

void XBeeRobot::FirmwareSPIPageProgramOperation::result() const {
	send_message_op.result();
}

void XBeeRobot::FirmwareSPIPageProgramOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSPIPageProgramOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareSPIPageProgramOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_PAGE_PROGRAM) {
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::FirmwareSPIBlockCRCOperation::FirmwareSPIBlockCRCOperation(XBeeRobot &robot, unsigned int address, std::size_t length) : robot(robot), address(address), length(length), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_CRC_BLOCK, static_cast<uint8_t>(address), static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address >> 16), static_cast<uint8_t>(length), static_cast<uint8_t>(length >> 8)}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	assert(1 <= length && length <= 65536);
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::FirmwareSPIBlockCRCOperation::send_message_op_done));
}

uint16_t XBeeRobot::FirmwareSPIBlockCRCOperation::result() const {
	send_message_op.result();
	return crc;
}

void XBeeRobot::FirmwareSPIBlockCRCOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareSPIBlockCRCOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareSPIBlockCRCOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	const uint8_t *pch = static_cast<const uint8_t *>(data);
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 8 && pch[0] == FIRMWARE_REQUEST_CRC_BLOCK && static_cast<uint32_t>(pch[1] | (pch[2] << 8) | (pch[3] << 16)) == address && static_cast<uint16_t>(pch[4] | (pch[5] << 8)) == (length & 0xFFFF)) {
			crc = static_cast<uint16_t>(pch[6] | (pch[7] << 8));
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::FirmwareReadOperationalParametersOperation::FirmwareReadOperationalParametersOperation(XBeeRobot &robot) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_READ_PARAMS}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::FirmwareReadOperationalParametersOperation::send_message_op_done));
}

XBeeRobot::OperationalParameters XBeeRobot::FirmwareReadOperationalParametersOperation::result() const {
	send_message_op.result();
	return params;
}

void XBeeRobot::FirmwareReadOperationalParametersOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareReadOperationalParametersOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareReadOperationalParametersOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	const uint8_t *pch = static_cast<const uint8_t *>(data);
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 7 && pch[0] == FIRMWARE_REQUEST_READ_PARAMS) {
#warning sanity checks
			params.flash_contents = flash_contents_of_int(pch[1]);
			params.xbee_channels[0] = pch[2];
			params.xbee_channels[1] = pch[3];
			params.robot_number = pch[4];
			params.dribble_power = pch[5];
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::FirmwareWriteOperationalParametersOperation::FirmwareWriteOperationalParametersOperation(XBeeRobot &robot, const OperationalParameters &params) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_SET_PARAMS, static_cast<uint8_t>(params.flash_contents), params.xbee_channels[0], params.xbee_channels[1], params.robot_number, params.dribble_power, 0}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::FirmwareWriteOperationalParametersOperation::send_message_op_done));
}

void XBeeRobot::FirmwareWriteOperationalParametersOperation::result() const {
	send_message_op.result();
}

void XBeeRobot::FirmwareWriteOperationalParametersOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareWriteOperationalParametersOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareWriteOperationalParametersOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_SET_PARAMS) {
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::FirmwareCommitOperationalParametersOperation::FirmwareCommitOperationalParametersOperation(XBeeRobot &robot) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_COMMIT_PARAMS}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::FirmwareCommitOperationalParametersOperation::send_message_op_done));
}

void XBeeRobot::FirmwareCommitOperationalParametersOperation::result() const {
	send_message_op.result();
}

void XBeeRobot::FirmwareCommitOperationalParametersOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareCommitOperationalParametersOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareCommitOperationalParametersOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_COMMIT_PARAMS) {
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::RebootOperation::RebootOperation(XBeeRobot &robot) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_REBOOT}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::RebootOperation::send_message_op_done));
}

void XBeeRobot::RebootOperation::result() const {
	send_message_op.result();
}

void XBeeRobot::RebootOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &RebootOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::RebootOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_REBOOT) {
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
}

XBeeRobot::FirmwareReadBuildSignaturesOperation::FirmwareReadBuildSignaturesOperation(XBeeRobot &robot) : robot(robot), buffer{static_cast<uint8_t>((robot.index << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_READ_BUILD_SIGNATURES}, send_message_op(robot.dongle, buffer, sizeof(buffer)) {
	send_message_op.signal_done.connect(sigc::mem_fun(this, &XBeeRobot::FirmwareReadBuildSignaturesOperation::send_message_op_done));
}

XBeeRobot::BuildSignatures XBeeRobot::FirmwareReadBuildSignaturesOperation::result() const {
	send_message_op.result();
	return sigs;
}

void XBeeRobot::FirmwareReadBuildSignaturesOperation::send_message_op_done(AsyncOperation<void> &op) {
	if (op.succeeded()) {
		receive_message_conn = robot.dongle.signal_message_received.connect(sigc::mem_fun(this, &FirmwareReadBuildSignaturesOperation::check_received_message));
	} else {
		signal_done.emit(*this);
	}
}

void XBeeRobot::FirmwareReadBuildSignaturesOperation::check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
	const uint8_t *pch = static_cast<const uint8_t *>(data);
	if (robot == this->robot.index && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
		if (len == 5 && pch[0] == FIRMWARE_REQUEST_READ_BUILD_SIGNATURES) {
#warning sanity checks
			sigs.firmware_signature = static_cast<uint16_t>(pch[1] | (pch[2] << 8));
			sigs.flash_signature = static_cast<uint16_t>(pch[3] | (pch[4] << 8));
			receive_message_conn.disconnect();
			signal_done.emit(*this);
		} else {
#warning TODO something sensible
		}
	}
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

void XBeeRobot::set_charger_state(ChargerState state) {
	drive_block.enable_charger = state == ChargerState::CHARGE;
	drive_block.float_capacitor_voltage = state != ChargerState::DISCHARGE;
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

	kick_send_message_op.reset(new XBeeDongle::SendMessageOperation(dongle, buffer, sizeof(buffer)));
	kick_send_message_op->signal_done.connect(sigc::mem_fun(this, &XBeeRobot::check_kick_message_result));
}

void XBeeRobot::test_mode(unsigned int mode) {
	assert((mode & 0xFFFF) == mode);

	XBeePackets::TestMode packet;
	packet.test_class = static_cast<uint8_t>(mode >> 8);
	packet.test_index = static_cast<uint8_t>(mode);

	uint8_t buffer[1 + packet.BUFFER_SIZE];
	buffer[0] = static_cast<uint8_t>((index << 4) | XBeeDongle::PIPE_TEST_MODE);
	packet.encode(buffer + 1);

	test_mode_send_message_op.reset(new XBeeDongle::SendMessageOperation(dongle, buffer, sizeof(buffer)));
	test_mode_send_message_op->signal_done.connect(sigc::mem_fun(this, &XBeeRobot::check_test_mode_message_result));
}

void XBeeRobot::start_experiment(uint8_t control_code) {
	uint8_t buffer[2];
	buffer[0] = static_cast<uint8_t>((index << 4) | XBeeDongle::PIPE_EXPERIMENT_CONTROL);
	buffer[1] = control_code;

	start_experiment_send_message_op.reset(new XBeeDongle::SendMessageOperation(dongle, buffer, sizeof(buffer)));
	start_experiment_send_message_op->signal_done.connect(sigc::mem_fun(this, &XBeeRobot::check_start_experiment_message_result));
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

void XBeeRobot::check_kick_message_result(AsyncOperation<void> &) {
	kick_send_message_op->result();
	kick_send_message_op.reset();
}

void XBeeRobot::check_test_mode_message_result(AsyncOperation<void> &) {
	test_mode_send_message_op->result();
	test_mode_send_message_op.reset();
}

void XBeeRobot::check_start_experiment_message_result(AsyncOperation<void> &) {
	start_experiment_send_message_op->result();
	start_experiment_send_message_op.reset();
}

