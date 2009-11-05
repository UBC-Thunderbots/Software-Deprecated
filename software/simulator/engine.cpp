#include "simulator/engine.h"
#include <stdexcept>

namespace {
	simulator_engine_factory::map_type &get_map() {
		static simulator_engine_factory::map_type objects;
		return objects;
	}
}

const simulator_engine_factory::map_type &simulator_engine_factory::all() {
	return get_map();
}

simulator_engine_factory::simulator_engine_factory(const Glib::ustring &name) : the_name(name) {
	if (get_map().count(name))
		throw std::logic_error(Glib::locale_from_utf8(Glib::ustring::compose("Duplicate simulator name \"%1\"", name)));
	get_map()[name] = this;
}

simulator_engine_factory::~simulator_engine_factory() {
	get_map().erase(name());
}

