#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

#include "simulator/team.h"
#include "util/exact_timer.h"

//
// The simulator itself.
//
// WARNING - WARNING - WARNING
// A lot of work is done in the constructors of this object and its contents.
// DO NOT CHANGE THE ORDER OF THE VARIABLES IN THIS CLASS.
// You may cause a segfault if you do.
//
class simulator : public playtype_source, public noncopyable, public sigc::trackable {
	public:
		//
		// Constructs a new simulator.
		//
		simulator(xmlpp::Element *xml);

		//
		// Gets the current engine.
		//
		simulator_engine::ptr get_engine() const {
			return engine;
		}

		//
		// Sets the engine to use to implement simulation.
		//
		void set_engine(const Glib::ustring &engine_name);

		//
		// Gets the current play type.
		//
		playtype::playtype current_playtype() const {
			return cur_playtype;
		}

		//
		// Returns a signal fired when the play type changes.
		//
		sigc::signal<void, playtype::playtype> &signal_playtype_changed() {
			return sig_playtype_changed;
		}

		//
		// Sets the current play type.
		//
		void set_playtype(playtype::playtype pt) {
			cur_playtype = pt;
			sig_playtype_changed.emit(pt);
		}

		//
		// The signal emitted after each timestep. This signal should only be
		// used to refresh the user interface or write data to a log; the actual
		// updating of the world state is done separately.
		//
		sigc::signal<void> &signal_updated() {
			return sig_updated;
		}

	private:
		//
		// The current play type.
		//
		playtype::playtype cur_playtype;

		//
		// Emitted when the play type changes.
		//
		sigc::signal<void, playtype::playtype> sig_playtype_changed;

	public:
		//
		// The field.
		//
		field::ptr fld;

		//
		// The ball, as viewed from the two ends of the field.
		//
		ball::ptr west_ball, east_ball;

		//
		// The two teams.
		//
		simulator_team_data west_team, east_team;

	private:
		//
		// The engine.
		//
		simulator_engine::ptr engine;

		//
		// The configuration element.
		//
		xmlpp::Element *xml;

		//
		// A callback invoked after each timestep.
		//
		sigc::signal<void> sig_updated;

		//
		// The timer.
		//
		exact_timer ticker;

		//
		// Handles a timer tick.
		//
		void tick();
};

#endif

