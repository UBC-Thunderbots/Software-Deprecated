#include "test/mrf/params.h"
#include "util/algorithm.h"
#include "util/string.h"
#include <cstddef>
#include <stdexcept>
#include <glibmm/main.h>
#include <gtkmm/messagedialog.h>
#include <sigc++/bind_return.h>
#include <sigc++/functors/mem_fun.h>

namespace {
	Glib::ustring format_channel(unsigned int ch) {
		return Glib::ustring::compose(u8"%1 (%2)", tohex(ch, 2), ch);
	}
}

ParamsPanel::ParamsPanel(MRFDongle &dongle, MRFRobot &robot) : Gtk::Table(4, 2), dongle(dongle), robot(robot), channel_label(u8"Channel:"), index_label(u8"Index:"), pan_label(u8"PAN (hex):") {
	reset_button_text();

	for (unsigned int ch = 0x0B; ch <= 0x1A; ++ch) {
		channel_chooser.append_text(format_channel(ch));
	}
	channel_chooser.set_active_text(format_channel(dongle.channel()));
	for (unsigned int i = 0; i <= 7; ++i) {
		index_chooser.append_text(Glib::ustring::format(i));
	}
	index_chooser.set_active_text(Glib::ustring::format(robot.index));
	pan_entry.set_width_chars(4);
	pan_entry.set_max_length(4);
	pan_entry.set_text(tohex(dongle.pan(), 4));
	set.signal_clicked().connect(sigc::mem_fun(this, &ParamsPanel::send_params));
	reboot.signal_clicked().connect(sigc::mem_fun(this, &ParamsPanel::reboot_robot));
	shut_down.signal_clicked().connect(sigc::mem_fun(this, &ParamsPanel::shut_down_robot));

	attach(channel_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channel_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index_chooser, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pan_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pan_entry, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	hbb.pack_start(set);
	hbb.pack_start(reboot);
	hbb.pack_start(shut_down);
	attach(hbb, 0, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
}

ParamsPanel::~ParamsPanel() {
	reset_button_connection.disconnect();
}

void ParamsPanel::activate_controls(bool act) {
	channel_chooser.set_sensitive(act);
	index_chooser.set_sensitive(act);
	pan_entry.set_sensitive(act);
	set.set_sensitive(act);
	reboot.set_sensitive(act);
	shut_down.set_sensitive(act);
}

void ParamsPanel::send_params() {
	if (channel_chooser.get_active_row_number() >= 0 && index_chooser.get_active_row_number() >= 0) {
		reset_button_connection.disconnect();
		reset_button_text();

		uint8_t channel = static_cast<uint8_t>(channel_chooser.get_active_row_number() + 0x0B);
		uint8_t index = static_cast<uint8_t>(index_chooser.get_active_row_number());
		uint16_t pan;
		try {
			pan = static_cast<uint16_t>(std::stoi(ustring2wstring(pan_entry.get_text()), nullptr, 16));
		} catch (const std::logic_error &) {
			pan = 0xFFFF;
		}
		if (pan == 0xFFFF) {
			Gtk::MessageDialog md(u8"Invalid PAN ID", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.run();
			return;
		}
		uint8_t packet[5];
		packet[0] = 0x0B;
		packet[1] = channel;
		packet[2] = index;
		packet[3] = static_cast<uint8_t>(pan >> 8);
		packet[4] = static_cast<uint8_t>(pan);
		set.set_label(u8"Sending…");
		activate_controls(false);
		message.reset(new MRFDongle::SendReliableMessageOperation(dongle, robot.index, packet, sizeof(packet)));
		message->signal_done.connect(sigc::mem_fun(this, &ParamsPanel::check_result));
		rebooting = false;
		shutting_down = false;
	}
}

void ParamsPanel::reboot_robot() {
	reset_button_connection.disconnect();
	reset_button_text();

	uint8_t packet[1];
	packet[0] = 0x08;
	reboot.set_label(u8"Sending…");
	activate_controls(false);
	message.reset(new MRFDongle::SendReliableMessageOperation(dongle, robot.index, packet, sizeof(packet)));
	message->signal_done.connect(sigc::mem_fun(this, &ParamsPanel::check_result));
	rebooting = true;
	shutting_down = false;
}

void ParamsPanel::shut_down_robot() {
	reset_button_connection.disconnect();
	reset_button_text();

	uint8_t packet[1];
	packet[0] = 0x0C;
	shut_down.set_label(u8"Sending…");
	activate_controls(false);
	message.reset(new MRFDongle::SendReliableMessageOperation(dongle, robot.index, packet, sizeof(packet)));
	message->signal_done.connect(sigc::mem_fun(this, &ParamsPanel::check_result));
	rebooting = false;
	shutting_down = true;
}

void ParamsPanel::check_result(AsyncOperation<void> &op) {
	Gtk::Button &btn = rebooting ? reboot : shutting_down ? shut_down : set;
	try {
		op.result();
		btn.set_label(u8"OK");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAssociatedError &) {
		btn.set_label(u8"Not associated");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAcknowledgedError &) {
		btn.set_label(u8"Not acknowledged");
	} catch (const MRFDongle::SendReliableMessageOperation::ClearChannelError &) {
		btn.set_label(u8"CCA fail");
	}
	message.reset();
	activate_controls(true);
	reset_button_connection = Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(this, &ParamsPanel::reset_button_text), false), 3U);
}

void ParamsPanel::reset_button_text() {
	set.set_label(u8"Set");
	reboot.set_label(u8"Reboot");
	shut_down.set_label(u8"Shut Down");
}

