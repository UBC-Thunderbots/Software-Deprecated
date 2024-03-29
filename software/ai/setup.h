#include <glibmm/ustring.h>
#include "ai/backend/backend.h"
#include "ai/common/colour.h"

namespace AI
{
/**
 * \brief The basic setup of the AI.
 */
class Setup final
{
   public:
    /**
     * \brief The name of the high level in use.
     */
    Glib::ustring high_level_name;

    /**
     * \brief The name of the navigator in use.
     */
    Glib::ustring navigator_name;

    /**
     * \brief Which end of the field the team is defending.
     */
    AI::BE::Backend::FieldEnd defending_end;

    /**
     * \brief The colour of the central dot on the team's robots.
     */
    AI::Common::Colour friendly_colour;

    /**
     * \brief Loads the cached setup data, if available.
     */
    explicit Setup();

    /**
     * \brief Saves the specified setup data into the cache file.
     */
    void save();
};
}
