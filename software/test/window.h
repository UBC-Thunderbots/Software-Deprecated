#ifndef TESTER_WINDOW_H
#define TESTER_WINDOW_H

#include "test/feedback.h"
#include "test/params.h"
#include "test/zeroable.h"
#include "uicomponents/annunciator.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * The user interface for the tester.
 */
class TesterWindow : public Gtk::Window {
	public:
		/**
		 * Creates a new TesterWindow.
		 *
		 * \param[in] dongle the dongle to use to talk to robots.
		 */
		TesterWindow(XBeeDongle &dongle);

	private:
		XBeeDongle &dongle;
		XBeeRobot::Ptr bot;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		Gtk::HBox bot_hbox;
		Gtk::ComboBoxText bot_chooser;
		Gtk::ToggleButton control_bot_button;

		Gtk::Frame feedback_frame;
		TesterFeedbackPanel feedback_panel;

		Gtk::Frame drive_frame;
		Gtk::VBox drive_box;
		Gtk::ComboBoxText drive_chooser;
		Gtk::Widget *drive_widget;
		Zeroable *drive_zeroable;

		Gtk::Frame dribble_frame;
		Gtk::ToggleButton dribble_button;

		Gtk::Frame chicker_frame;
		Gtk::Table chicker_table;
		Gtk::CheckButton chicker_enabled;
		Gtk::Label chicker_pulse_width1_label;
		Gtk::HScale chicker_pulse_width1;
		Gtk::Label chicker_pulse_width2_label;
		Gtk::HScale chicker_pulse_width2;
		Gtk::Label chicker_pulse_offset_label;
		Gtk::HScale chicker_pulse_offset;
		Gtk::HBox chicker_fire_hbox;
		Gtk::Button chicker_kick;
		Gtk::ToggleButton chicker_autokick;

		Gtk::Frame params_frame;
		TesterParamsPanel params_panel;

		GUIAnnunciator ann;

		sigc::connection bot_alive_changed_signal;

		int key_snoop(Widget *, GdkEventKey *event);
		void on_control_toggled();
		void on_bot_alive_changed();
		void on_bot_claim_failed_locked();
		void on_bot_claim_failed_resource();
		void on_bot_claim_failed(const Glib::ustring &);
		void drive_mode_changed();
		void on_dribble_toggled();
		void on_chicker_enable_change();
		void on_chicker_pulse_width_changed();
		void on_chicker_pulse_offset_changed();
		void on_chicker_kick();
		void on_chicker_autokick();
};

#endif

