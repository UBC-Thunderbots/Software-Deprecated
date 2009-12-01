#include "simulator/field.h"
#include "simulator/simulator.h"
#include "util/xml.h"
#include "world/config.h"

simulator::simulator(xmlpp::Element *xml, clocksource &clk) : cur_playtype(playtype::halt), the_ball_impl(simulator_ball_impl::trivial()), fld(new simulator_field), west_ball(new ball(the_ball_impl, false)), east_ball(new ball(the_ball_impl, true)), west_team(*this, false, xmlutil::strip(xmlutil::get_only_child(xml, "westteam")), true, west_ball, fld), east_team(*this, true, xmlutil::strip(xmlutil::get_only_child(xml, "eastteam")), false, east_ball, fld), xml(xml) {
	// Configure objects with each other as the opponents.
	west_team.set_other(east_team.west_view, east_team.east_view);
	east_team.set_other(west_team.west_view, west_team.east_view);

	// Set the appropriate engine.
	xmlpp::Element *xmlengines = xmlutil::strip(xmlutil::get_only_child(xml, "engines"));
	set_engine(xmlengines->get_attribute_value("active"));

	// Set the appropriate autoref.
	xmlpp::Element *xmlautorefs = xmlutil::strip(xmlutil::get_only_child(xml, "autorefs"));
	set_autoref(xmlautorefs->get_attribute_value("active"));

	// Connect to the update tick.
	clk.signal_tick().connect(sigc::mem_fun(*this, &simulator::tick));
}

void simulator::set_engine(const Glib::ustring &engine_name) {
	// Get the name of the current autoref.
	const Glib::ustring &autoref_name = ref ? ref->get_factory().name() : "No Autoref";

	// Get the "engines" XML element.
	xmlpp::Element *xmlengines = xmlutil::get_only_child(xml, "engines");

	// Find the engine factory for this engine type.
	const simulator_engine_factory::map_type &factories = simulator_engine_factory::all();
	simulator_engine_factory::map_type::const_iterator factoryiter = factories.find(engine_name);
	if (factoryiter != factories.end()) {
		simulator_engine_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlengines, "params", "engine", engine_name));
		engine = factory->create_engine(xmlparams);
	} else {
		engine.reset();
	}

	// Select the proper implementation of the ball.
	if (engine) {
		the_ball_impl = engine->get_ball();
	} else {
		the_ball_impl = simulator_ball_impl::trivial();
	}
	west_ball->set_impl(the_ball_impl);
	east_ball->set_impl(the_ball_impl);

	// Make the teams use the proper engine.
	west_team.set_engine(engine);
	east_team.set_engine(engine);

	// Save the choice of engine in the configuration.
	if (xmlengines->get_attribute_value("active") != engine_name) {
		xmlengines->set_attribute("active", engine_name);
		config::dirty();
	}

	// Recreate the autoref so that it gets the new objects.
	set_autoref(autoref_name);
}

void simulator::set_autoref(const Glib::ustring &autoref_name) {
	// Get the "autorefs" XML element.
	xmlpp::Element *xmlautorefs = xmlutil::get_only_child(xml, "autorefs");

	// Find the autoref factory for this autoref type.
	const autoref_factory::map_type &factories = autoref_factory::all();
	autoref_factory::map_type::const_iterator factoryiter = factories.find(autoref_name);
	if (factoryiter != factories.end()) {
		autoref_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlautorefs, "params", "autoref", autoref_name));
		ref = factory->create_autoref(*this, the_ball_impl, west_team.impls, east_team.impls, xmlparams);
	} else {
		ref.reset();
	}

	// Save the choice of autoref in the configuration.
	if (xmlautorefs->get_attribute_value("active") != autoref_name) {
		xmlautorefs->set_attribute("active", autoref_name);
		config::dirty();
	}
}

void simulator::tick() {
	west_team.tick_preengine();
	east_team.tick_preengine();
	if (engine)
		engine->tick();
	west_team.tick_postengine();
	east_team.tick_postengine();
	the_ball_impl->add_prediction_datum(the_ball_impl->position());
	if (ref)
		ref->tick();
}

