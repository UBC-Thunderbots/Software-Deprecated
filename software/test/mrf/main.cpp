#include "test/mrf/launcher.h"
#include "util/annunciator.h"
#include "util/config.h"
#include "mrf/dongle.h"
#include <iostream>
#include <locale>
#include <stdexcept>
#include <gtkmm/main.h>

namespace {
	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary(u8"Allows testing robot subsystems.");

		// Load the configuration file.
		Config::load();

		// Initialize GTK.
		Gtk::Main m(argc, argv, option_context);
		if (argc != 1) {
			std::cout << option_context.get_help();
			return 1;
		}

		// Find and open the dongle.
		std::cout << "Finding dongle... " << std::flush;
		MRFDongle dongle;
		std::cout << "OK\n";

		// Create the window.
		TesterLauncher win(dongle);
		Gtk::Main::run(win);

		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}
