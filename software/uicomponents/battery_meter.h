#ifndef UICOMPONENTS_BATTERY_METER_H
#define UICOMPONENTS_BATTERY_METER_H

#include "util/noncopyable.h"
#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the battery level of a robot.
//
class BatteryMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		//
		// Constructs a BatteryMeter with no robot.
		//
		BatteryMeter();

		//
		// Sets which robot this battery meter will monitor.
		//
		void set_bot(RefPtr<XBeeDriveBot> bot);

	private:
		RefPtr<XBeeDriveBot> robot;
		sigc::connection connection;
		unsigned int last_voltage;

		void update();
};

#endif

