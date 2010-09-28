#ifndef FIRMWARE_WINDOW_H
#define FIRMWARE_WINDOW_H

#include "uicomponents/single_bot_combobox.h"
#include "util/config.h"
#include "util/scoped_ptr.h"
#include "xbee/client/lowlevel.h"
#include <gtkmm.h>
#include <string>

/**
 * The user interface for the firmware manager.
 */
class FirmwareWindow : public Gtk::Window {
	public:
		/**
		 * Constructs a new FirmwareWindow.
		 *
		 * \param[in] modem the local modem to use to communicate with robots.
		 *
		 * \param[in] conf the configuration file listing the robots.
		 *
		 * \param[in] robot the pattern of the robot to pre-select in the list, or \c -1 to not pre-select any robot.
		 *
		 * \param[in] filename the filename to pre-select in the chooser, or empty to not pre-select any file.
		 */
		FirmwareWindow(XBeeLowLevel &modem, const Config &conf, int robot, const std::string &filename);

	private:
		XBeeLowLevel &modem;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		SingleBotComboBox bot_controls;

		Gtk::Frame file_frame;
		Gtk::VBox file_vbox;
		Gtk::FileChooserButton file_chooser;
		Gtk::HBox file_target_hbox;
		Gtk::RadioButtonGroup file_target_group;
		Gtk::RadioButton file_fpga_button, file_pic_button;

		Gtk::Button start_upload_button;
		Gtk::Button emergency_erase_button;

		bool on_delete_event(GdkEventAny *);
		void start_upload();
		void start_emergency_erase();
};

#endif

