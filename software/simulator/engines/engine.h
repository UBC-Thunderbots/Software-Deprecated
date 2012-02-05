#ifndef SIM_ENGINES_ENGINE_H
#define SIM_ENGINES_ENGINE_H

#include "simulator/ball.h"
#include "simulator/player.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/registerable.h"
#include <memory>

namespace Gtk {
	class Widget;
}
class SimulatorEngineFactory;

/**
 * A simulation engine.
 * Individual simulation engines should extend this class to provide actual simulation services.
 */
class SimulatorEngine : public NonCopyable {
	public:
		/**
		 * Runs a time tick.
		 * The engine should update the positions of all its players and the ball.
		 */
		virtual void tick() = 0;

		/**
		 * Retrieves the engine's specific SimulatorBall object.
		 *
		 * \return the SimulatorBall object.
		 */
		virtual Simulator::Ball &get_ball() = 0;

		/**
		 * Creates a new SimulatorPlayer.
		 * The engine must keep a pointer to the new object so that SimulatorEngine::tick() can move the SimulatorPlayer.
		 *
		 * \return the new SimulatorPlayer object.
		 */
		virtual Simulator::Player *add_player() = 0;

		/**
		 * Removes from the simulation an existing SimulatorPlayer.
		 *
		 * \param[in] player the SimulatorPlayer to remove.
		 */
		virtual void remove_player(Simulator::Player *player) = 0;

		/**
		 * Loads a simulation engine state from a file.
		 *
		 * \param[in] fd the file to load from.
		 */
		virtual void load_state(const FileDescriptor &fd) = 0;

		/**
		 * Saves the current state of the simulation engine into a file.
		 *
		 * \param[in] fd the file to store into.
		 */
		virtual void save_state(const FileDescriptor &fd) const = 0;

		/**
		 * Retrieves the factory object that created the engine.
		 *
		 * \return the engine factory that created the engine.
		 */
		virtual SimulatorEngineFactory &get_factory() = 0;
};

/**
 * A factory for creating \ref SimulatorEngine "SimulatorEngines".
 * An individual implementation should extend this class to provide a class which can create objects of a particular derived implementation of SimulatorEngine,
 * and then create a single instance of the factory in a global variable.
 */
class SimulatorEngineFactory : public Registerable<SimulatorEngineFactory> {
	public:
		/**
		 * Constructs a new SimulatorEngine.
		 *
		 * \return the new engine.
		 */
		virtual std::unique_ptr<SimulatorEngine> create_engine() = 0;

	protected:
		/**
		 * Constructs a SimulatorEngineFactory.
		 * This should be invoked at application startup (by creating a global variable instance of the implementing class) to register the factory.
		 *
		 * \param[in] name a short string naming the factory.
		 */
		SimulatorEngineFactory(const char *name) : Registerable<SimulatorEngineFactory>(name) {
		}
};

#endif

