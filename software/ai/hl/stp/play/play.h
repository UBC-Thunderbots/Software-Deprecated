#ifndef AI_HL_STP_PLAY_PLAY_H
#define AI_HL_STP_PLAY_PLAY_H

#include "ai/hl/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <sigc++/sigc++.h>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Play {

				class PlayFactory;

				/**
				 * A play is a level in the STP paradigm.
				 * The purpose of a play is to produce sequences of tactics.
				 *
				 * Note that there is only one instance per class.
				 * Plays are reused, destroyed only when the AI ends.
				 * Doing so will simplify coding.
				 */
				class Play : public ByRef, public sigc::trackable {
					public:
						typedef RefPtr<Play> Ptr;

						/**
						 * Checks if this play is applicable.
						 * A subclass must implement this function.
						 */
						virtual bool applicable() const = 0;

						/**
						 * Called when this play is first used or reset.
						 * Since a play is reusable,
						 * this is the place to initialize variables.
						 */
						virtual void initialize() = 0;

						/**
						 * Provide sequences of tactics.
						 * A subclass must implement this function.
						 *
						 * \param [in] goalie_role a sequence of tactics for the goalie
						 *
						 * \param [in] an array of tactic sequences in order of priority.
						 * The first entry is the most important.
						 * Each entry is a sequence of tactics.
						 */
						virtual void assign(std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &goalie_role, std::vector<AI::HL::STP::Tactic::Tactic::Ptr>* roles) = 0;

						/**
						 * Checks if the condition for the play is no longer valid.
						 */
						virtual bool done() = 0;

						/**
						 * A reference to this play's factory.
						 */
						virtual const PlayFactory& factory() const = 0;

					protected:
						/**
						 * The World in which the Play lives.
						 */
						AI::HL::W::World &world;

						/**
						 * The constructor.
						 * You should initialize variables in the initialize() function.
						 */
						Play(AI::HL::W::World &world);

						/**
						 * Destructor
						 */
						virtual ~Play();
				};

				/**
				 * A PlayFactory is used to construct a particular type of Play.
				 * The factory permits STP to discover all available types of Plays.
				 */
				class PlayFactory : public Registerable<PlayFactory> {
					public:
						/**
						 * Constructs a new instance of the Play corresponding to this PlayManager.
						 */
						virtual Play::Ptr create(AI::HL::W::World &world) const = 0;

						/**
						 * Constructs a new PlayFactory.
						 * Subclasses should call this constructor from their own constructors.
						 *
						 * \param[in] name a human-readable name for this Play.
						 */
						PlayFactory(const char *name);
				};
			}
		}
	}
}

#endif

