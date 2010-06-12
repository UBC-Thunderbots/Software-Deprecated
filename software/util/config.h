#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include <istream>
#include <ostream>
#include <vector>
#include <glibmm.h>
#include <stdint.h>
#include "util/noncopyable.h"

//
// Provides access to the configuration file.
//
class config : public noncopyable {
	public:
		//
		// The configuration of a single robot.
		//
		struct robot_info {
			uint64_t address;
			bool yellow;
			unsigned int pattern_index;
			Glib::ustring name;

			robot_info() {
			}

			robot_info(uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name) : address(address), yellow(yellow), pattern_index(pattern_index), name(name) {
			}
		};

		//
		// A collection of robot_info objects.
		//
		class robot_set : public noncopyable {
			public:
				//
				// Returns the number of robots in the list.
				//
				unsigned int size() const {
					return robots.size();
				}

				//
				// Gets a robot by its position in the list.
				//
				const robot_info &operator[](unsigned int index) const {
					return robots[index];
				}

				//
				// Gets a robot by its 64-bit address.
				//
				const robot_info &find(uint64_t address) const;

				/**
				 * Gets a robot by its name.
				 *
				 * \param[in] name the name of the robot.
				 *
				 * \return the robot's information structure.
				 */
				const robot_info &find(const Glib::ustring &name) const;

				//
				// Checks whether a particular address is already in the list.
				//
				bool contains_address(uint64_t address) const;

				//
				// Checks whether a particular vision pattern is already in the
				// list.
				//
				bool contains_pattern(bool yellow, unsigned int pattern_index) const;

				/**
				 * \param[in] name the name to look for.
				 *
				 * \return \c true if \p name is already used by a robot in this
				 * collection, or \c false if not.
				 */
				bool contains_name(const Glib::ustring &name) const;

				//
				// Adds a new robot.
				//
				void add(uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name);

				//
				// Emitted when a robot is added. Parameter is the index of the
				// robot.
				//
				mutable sigc::signal<void, unsigned int> signal_robot_added;

				//
				// Deletes a robot.
				//
				void remove(uint64_t address);

				//
				// Emitted when a robot is deleted. Parameter is the index of
				// the robot.
				//
				mutable sigc::signal<void, unsigned int> signal_robot_removed;

				/**
				 * Replaces an existing robot with new data.
				 *
				 * \param[in] old_address the XBee address of the robot to
				 * replace.
				 *
				 * \param[in] address the new XBee address to store.
				 *
				 * \param[in] yellow the new colour of the robot.
				 *
				 * \param[in] pattern_index the new lid pattern index.
				 *
				 * \param[in] name the new name.
				 */
				void replace(uint64_t old_address, uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name);

				/**
				 * Emitted when a robot is replaced. Parameter is the index of
				 * the robot.
				 */
				mutable sigc::signal<void, unsigned int> signal_robot_replaced;

				/**
				 * Sorts the robots in the collection by their 64-bit address.
				 */
				void sort_by_address();

				/**
				 * Sorts the robots in the collection by their lid pattern.
				 */
				void sort_by_lid();

				/**
				 * Sorts the robots in the collection by their name.
				 */
				void sort_by_name();

				/**
				 * Emitted when the collection is sorted.
				 */
				mutable sigc::signal<void> signal_sorted;

				/**
				 * Inverts the colours of all robots in the collection.
				 */
				void swap_colours();

				/**
				 * Emitted when the colours of all robots are swapped.
				 */
				mutable sigc::signal<void> signal_colours_swapped;

			private:
				std::vector<robot_info> robots;

				void save(std::ostream &ofs) const;
				void load_v1(std::istream &ifs);
				void load_v2(std::istream &ifs);

				friend class config;
		};

		//
		// Loads the configuration file.
		//
		config();

		//
		// Saves the configuration file.
		//
		void save() const;

		//
		// Returns the set of configured robots.
		//
		const robot_set &robots() const {
			return robots_;
		}

		//
		// Returns the set of configured robots.
		//
		robot_set &robots() {
			return robots_;
		}

		/**
		 * \return the radio channel.
		 */
		unsigned int channel() const {
			return channel_;
		}

		/**
		 * Sets the radio channel.
		 *
		 * \param[in] chan the new channel, which must be between \c 0x0B and \c
		 * 0x1A.
		 */
		void channel(unsigned int chan);

	private:
		robot_set robots_;
		unsigned int channel_;

		void load_v1(std::istream &ifs);
		void load_v2(std::istream &ifs);
};

#endif

