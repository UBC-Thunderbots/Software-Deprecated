#include "util/dprint.h"
#include <ctime>
#include <cwchar>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

Glib::ustring todec(uintmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

Glib::ustring tohex(uintmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::hex | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

sigc::signal<void, unsigned int, const Glib::ustring &> signal_message_logged;

void log_impl(const char *file, unsigned int line, const Glib::ustring &msg, unsigned int level) {
	std::time_t stamp;
	std::time(&stamp);
	std::wostringstream timestring;
	static const wchar_t TIME_PATTERN[] = L"%F %T";
	std::use_facet<std::time_put<wchar_t> >(std::locale()).put(timestring, timestring, L' ', std::localtime(&stamp), TIME_PATTERN, TIME_PATTERN + std::wcslen(TIME_PATTERN));
	const char *level_name;
	switch (level) {
		case LOG_LEVEL_INFO:
			level_name = "INFO";
			break;

		case LOG_LEVEL_WARN:
			level_name = "WARN";
			break;

		case LOG_LEVEL_ERROR:
			level_name = "ERROR";
			break;

		default:
			level_name = 0;
			break;
	}
	const Glib::ustring &composed = Glib::ustring::compose("[%1] [%2:%3] %4", timestring.str(), file, line, msg);
	if (level_name) {
		std::cout << level_name << ' ' << composed << '\n';
	}
	signal_message_logged.emit(level, composed);
}

