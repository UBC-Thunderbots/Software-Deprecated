#include "fw/ihex.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <stdint.h>
#include <string>

namespace {
	void stripws(std::string &s) {
		// Find the first non-whitespace character.
		std::string::iterator i = std::find_if(s.begin(), s.end(), [](char ch) { return !std::isspace(ch); });

		// Erase everything up to and not including the first non-whitespace character.
		s.erase(s.begin(), i);

		// Erase the suffix that is whitespace.
		while (s.size() > 0 && std::isspace(s[s.size() - 1])) {
			s.erase(s.size() - 1);
		}
	}

	uint8_t decode_hex_nybble(char ch) {
		if (ch >= '0' && ch <= '9') {
			return static_cast<uint8_t>(ch - '0');
		} else if (ch >= 'A' && ch <= 'F') {
			return static_cast<uint8_t>(ch - 'A' + 0xA);
		} else if (ch >= 'a' && ch <= 'f') {
			return static_cast<uint8_t>(ch - 'a' + 0xA);
		} else {
			throw Firmware::MalformedHexFileError();
		}
	}

	void decode_line_data(std::vector<uint8_t> &out, const std::string &in) {
		for (std::size_t i = 0; i < in.size(); i += 2) {
			out.push_back(static_cast<uint8_t>(decode_hex_nybble(in[i]) * 16 + decode_hex_nybble(in[i + 1])));
		}
	}

	void check_checksum(const std::vector<uint8_t> &data) {
		if (std::accumulate(data.begin(), data.end(), static_cast<uint8_t>(0)) != 0) {
			throw Firmware::MalformedHexFileError();
		}
	}
}

Firmware::MalformedHexFileError::MalformedHexFileError() : std::runtime_error("Malformed hex file") {
}

void Firmware::IntelHex::add_section(unsigned int start, unsigned int length) {
	sections.push_back(Section(start, length));
}

void Firmware::IntelHex::load(const std::string &filename) {
	// Allocate space to hold the new data.
	std::vector<std::vector<uint8_t> > new_data(sections.size());

	// Open the file.
	std::ifstream ifs;
	ifs.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	ifs.open(filename.c_str());

	// Remember the current "base address".
	uint32_t address_base = 0;
	bool eof = false;
	while (!eof) {
		// Read a line from the hex file.
		std::string line;
		try {
			std::getline(ifs, line);
		} catch (...) {
			// Check for EOF (we should have seen an EOF record in the file).
			if (ifs.eof()) {
				throw MalformedHexFileError();
			} else {
				throw;
			}
		}

		// Strip any whitespace.
		stripws(line);

		// Check that the line starts with a colon, and remove it.
		if (line[0] != ':') {
			throw MalformedHexFileError();
		}
		line.erase(0, 1);

		// Check that the line is an even length (whole count of bytes).
		if (line.size() % 2 != 0) {
			throw MalformedHexFileError();
		}

		// Decode the line into bytes.
		std::vector<uint8_t> line_data;
		decode_line_data(line_data, line);

		// Check size.
		if (line_data.size() < 5) {
			throw MalformedHexFileError();
		}
		if (line_data.size() != 1U + 2U + 1U + line_data[0] + 1U) {
			throw MalformedHexFileError();
		}

		// Check the checksum.
		check_checksum(line_data);

		// Handle the record.
		uint8_t data_length = line_data[0];
		unsigned int record_address = line_data[1] * 256 + line_data[2];
		uint8_t record_type = line_data[3];
		const uint8_t *record_data = &line_data[4];
		if (record_type == 0x00) {
			// Data record.
			unsigned int real_address = address_base + record_address;
			for (uint8_t i = 0; i < data_length; ++i) {
				unsigned int byte_address = real_address + i;
				bool found = false;
				for (std::size_t j = 0; j < sections.size(); ++j) {
					const Section &sec = sections[j];
					if (sec.start() <= byte_address && byte_address < sec.start() + sec.length()) {
						found = true;
						std::vector<uint8_t> &d = new_data[j];
						unsigned int offset = byte_address - sec.start();
						while (d.size() <= offset) {
							d.push_back(0xFF);
						}
						d[offset] = record_data[i];
					}
				}
				if (!found) {
					throw MalformedHexFileError();
				}
			}
		} else if (record_type == 0x01) {
			// EOF record.
			if (data_length != 0) {
				throw MalformedHexFileError();
			}
			eof = true;
		} else if (record_type == 0x02) {
			// Extended Segment Address record.
			if (data_length != 2 || record_address != 0) {
				throw MalformedHexFileError();
			}
			address_base = (record_data[0] * 256 + record_data[1]) * 16;
		} else if (record_type == 0x03) {
			// Start Segment Address record. Ignored.
		} else if (record_type == 0x04) {
			// Extended Linear Address record.
			if (data_length != 2 || record_address != 0) {
				throw MalformedHexFileError();
			}
			address_base = (record_data[0] * 256 + record_data[1]) * 65536;
		} else if (record_type == 0x05) {
			// Start Linear Address record. Ignored.
		} else {
			throw MalformedHexFileError();
		}
	}

	// Load successful.
	the_data = new_data;
}

