#include "main.h"
#include "util/algorithm.h"
#include "util/annunciator.h"
#include "util/crc16.h"
#include "util/main_loop.h"
#include "util/noncopyable.h"
#include "util/string.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <iostream>
#include <locale>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>
#include <glibmm/main.h>
#include <glibmm/ustring.h>

namespace {
	class RobotExperimentReceiver : public NonCopyable, public sigc::trackable {
		public:
			RobotExperimentReceiver(uint8_t control_code, XBeeRobot &robot) : control_code(control_code), robot(robot) {
				std::fill(received, received + G_N_ELEMENTS(received), false);
				robot.signal_experiment_data.connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_experiment_data));
				Glib::signal_idle().connect_once(sigc::mem_fun(this, &RobotExperimentReceiver::start_operation));
			}

		private:
			uint8_t control_code;
			XBeeRobot &robot;
			sigc::connection alive_changed_connection;
			uint8_t data[256];
			bool received[256];

			void start_operation() {
				std::cerr << "Waiting for robot to appear... " << std::flush;
				alive_changed_connection = robot.alive.signal_changed().connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_alive_changed));
				on_alive_changed();
			}

			void on_alive_changed() {
				if (robot.alive) {
					alive_changed_connection.disconnect();
					alive_changed_connection = robot.alive.signal_changed().connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_alive_changed2));
					std::cerr << "OK\n";
					std::cerr << "Issuing control code... " << std::flush;
					robot.start_experiment(control_code);
					std::cerr << "OK\n";
				}
			}

			void on_alive_changed2() {
				if (!robot.alive) {
					std::cerr << "Robot unexpectedly died\n";
					MainLoop::quit();
				}
			}

			void on_experiment_data(const void *vp, std::size_t len) {
				const uint8_t *p = static_cast<const uint8_t *>(vp);
				if (p[0] + len - 1 <= G_N_ELEMENTS(received)) {
					std::copy(p + 1, p + len, data + p[0]);
					std::fill(received + p[0], received + p[0] + len - 1, true);
					std::ptrdiff_t c = std::count(received, received + G_N_ELEMENTS(received), true);
					if (c == G_N_ELEMENTS(received)) {
						std::cerr << "Done!\n";
						for (std::size_t i = 0; i < G_N_ELEMENTS(data); ++i) {
							std::cout << static_cast<unsigned int>(data[i]) << '\n';
						}
						MainLoop::quit();
					} else {
						std::cerr << c << " / " << G_N_ELEMENTS(received) << '\n';
					}
				}
			}
	};
}

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Handle command-line arguments.
	if (argc != 3) {
		std::cerr << "Usage:\n  " << argv[0] << " ROBOT_INDEX CONTROL_CODE\n\nLaunches a controlled experiment.\n\nApplication Options:\n  ROBOT_INDEX     Selects the robot to run the experiment.\n  CONTROL_CODE    Specifies a one-byte control code to select an experiment to run\n\n";
		return 1;
	}
	unsigned int robot_index;
	{
		Glib::ustring str(argv[1]);
		std::wistringstream iss(ustring2wstring(str));
		iss >> robot_index;
		if (!iss || robot_index > 15) {
			std::cerr << "Invalid robot index (must be a decimal number from 0 to 15).\n";
			return 1;
		}
	}
	uint8_t control_code;
	{
		Glib::ustring str(argv[2]);
		if (str.size() != 2 || !std::isxdigit(static_cast<wchar_t>(str[0]), std::locale()) || !std::isxdigit(static_cast<wchar_t>(str[1]), std::locale())) {
			std::cerr << "Invalid control code (must be a 2-digit hex number).\n";
			return 1;
		}
		std::wistringstream iss(ustring2wstring(str));
		iss.setf(std::ios_base::hex, std::ios_base::basefield);
		unsigned int ui;
		iss >> ui;
		control_code = static_cast<uint8_t>(ui);
	}

	std::cerr << "Finding and resetting dongle... " << std::flush;
	XBeeDongle dongle(true);
	std::cerr << "OK\n";

	std::cerr << "Enabling radios... " << std::flush;
	dongle.enable();
	std::cerr << "OK\n";

	RobotExperimentReceiver rx(control_code, dongle.robot(robot_index));
	MainLoop::run();

	return 0;
}

