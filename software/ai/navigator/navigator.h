#ifndef AI_NAVIGATOR_NAVIGATOR_H
#define AI_NAVIGATOR_NAVIGATOR_H

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <memory>
#include <utility>
#include "ai/navigator/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include "util/registerable.h"

namespace Gtk
{
class Widget;
}

namespace AI
{
namespace Nav
{
class NavigatorFactory;

/**
 * \brief A Navigator path-plans a team on behalf of a strategy.
 *
 * To implement a Navigator, one must:
 * <ul>
 * <li>Subclass Navigator</li>
 * <li>In the subclass, override all the pure virtual functions</li>
 * <li>Subclass NavigatorFactory</li>
 * <li>In the subclass, override all the pure virtual functions</li>
 * <li>Create an instance of the NavigatorFactory in the form of a file-scope
 * global variable</li>
 * </ul>
 */
class Navigator : public NonCopyable
{
   public:
    /**
     * \brief A pointer to a Navigator object.
     */
    typedef std::unique_ptr<Navigator> Ptr;

    /**
     * \brief Destroys a Navigator.
     */
    virtual ~Navigator();

    /**
     * \brief Finds the NavigatorFactory that constructed this Navigator.
     *
     * Subclasses must override this function to return a reference to the
     * global instance of their corresponding NavigatorFactory.
     *
     * \return a reference to the NavigatorFactory instance.
     */
    virtual NavigatorFactory &factory() const = 0;

    /**
     * \brief Reads the requested destinations from the players using
     * W::Player::destination and W::Player::flags, then orders paths to be
     * followed with W::Player::path.
     */
    virtual void tick() = 0;

    /**
     * \brief Returns the GTK widget for this Navigator, which will be
     * integrated into the AI's user interface.
     *
     * \return a GUI widget containing the controls for this Navigator,
     * or a null pointer if no GUI widgets are needed for this Navigator.
     *
     * \note The default implementation returns a null pointer.
     */
    virtual Gtk::Widget *ui_controls();

    /**
     * \brief Provides an opportunity for the navigator to draw an overlay on
     * the visualizer.
     *
     * The default implementation does nothing.
     * A subclass should override this function if it wishes to draw an overlay.
     *
     * \param[in] ctx the Cairo context onto which to draw.
     */
    virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);

   protected:
    /**
     * \brief The World in which to navigate.
     */
    AI::Nav::W::World world;

    /**
     * \brief Constructs a new Navigator.
     *
     * \param[in] world the World in which to navigate.
     */
    explicit Navigator(AI::Nav::W::World world);

    // hack should move this variable somewhere else later, koko
    // save state for new_pivot
    bool is_last_ccw;
};

/**
 * \brief A NavigatorFactory is used to construct a particular type of
 * Navigator.
 *
 * The factory permits the UI to discover all the available types of Navigator.
 */
class NavigatorFactory : public Registerable<NavigatorFactory>
{
   public:
    /**
     * \brief Constructs a new instance of the Navigator corresponding to this
     * NavigatorFactory.
     *
     * \param[in] world the World in which the new Navigator should live.
     *
     * \return the new Navigator.
     */
    virtual Navigator::Ptr create_navigator(AI::Nav::W::World world) const = 0;

   protected:
    /**
     * \brief Constructs a new NavigatorFactory.
     *
     * Subclasses should call this constructor from their own constructors.
     *
     * \param[in] name a human-readable name for this Navigator.
     */
    explicit NavigatorFactory(const char *name);
};
}
}

#define NAVIGATOR_REGISTER(cls)                                                \
    namespace                                                                  \
    {                                                                          \
    class cls##NavigatorFactory final : public AI::Nav::NavigatorFactory       \
    {                                                                          \
       public:                                                                 \
        explicit cls##NavigatorFactory();                                      \
        std::unique_ptr<Navigator> create_navigator(                           \
            AI::Nav::W::World) const override;                                 \
    };                                                                         \
    }                                                                          \
                                                                               \
    cls##NavigatorFactory::cls##NavigatorFactory() : NavigatorFactory(#cls)    \
    {                                                                          \
    }                                                                          \
                                                                               \
    std::unique_ptr<Navigator> cls##NavigatorFactory::create_navigator(        \
        AI::Nav::W::World world) const                                         \
    {                                                                          \
        std::unique_ptr<Navigator> p(new cls(world));                          \
        return p;                                                              \
    }                                                                          \
                                                                               \
    cls##NavigatorFactory cls##NavigatorFactory_instance;                      \
                                                                               \
    NavigatorFactory &cls::factory() const                                     \
    {                                                                          \
        return cls##NavigatorFactory_instance;                                 \
    }

#endif
