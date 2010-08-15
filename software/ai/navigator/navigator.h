#ifndef AI_NAVIGATOR_NAVIGATOR_H
#define AI_NAVIGATOR_NAVIGATOR_H

#include "util/registerable.h"
#include "ai/world/player.h"
#include "ai/world/robot.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include <utility>
#include <map>
#include <glibmm.h>

/**
 * A player's view of navigation.
 *
 * The TeamNavigator has all the copies of individual navigators there is a
 * one-to-one relation between players and Navigators.
 */
class Navigator : public ByRef{
	public:
		/**
		 * A pointer to a Navigator object.
		 */
		typedef RefPtr<Navigator> Ptr;
		
		/**
		 * The player being navigated.
		 */
		const Player::Ptr the_player;

		/**
		 * The world in which to navigate.
		 */
		const World::Ptr the_world;	
	
		/**
		 * Constructs a new Navigator.
		 *
		 * \param[in] player the Player to navigate.
		 *
		 * \param[in] world the World in which to navigate.
		 */
		Navigator(Player::Ptr player, World::Ptr world):the_player(player), the_world(world){
		}

		/**
		 * Sets the desired location.
		 *
		 * \param[in] position the desired target for the robot.
		 *
		 * \param[in] error the radius of a circle where the robot is "close
		 * enough" to its target.
		 */
		void set_position(const Point& position, double error) {
			target_position.first = position;
			target_position.second = error;
		}

		/**
		 * Normally the navigator sets the robot orientation to be towards the
		 * ball. Use this if you want to override this behaviour. This only sets
		 * the desired orientation for one timestep. You have to call this
		 * function every timestep.
		 *
		 * \param[in] orientation the desired orientation for the robot.
		 *
		 * \param[in] error the number of radians inside of which the robot is
		 * "close enough" to its target orientaton.
		 */
		void set_orientation(const double& orientation, double error) {
			target_orientation.first = orientation;
			target_orientation.second = error;
		}

		/**
		 * Turns on dribbler at minimal speed and be ready to dribble to Receive
		 * the ball.
		 *
		 * You need to call this every tick.
		 *
		 * I don't think you ever want to turn this off once you turn it on.
		 *
		 * \param[in] dribble \c true to turn the dribbler on, or \c false to
		 * turn it off.
		 */
		void set_dribbler(bool dribble) {
			need_dribble = dribble;
		}

		/**
		 * Sets flags.
		 *
		 * \param[in] f the flags to enable.
		 */
		void set_flags(unsigned int f) {
			flags |= f;
		}
		
		/**
		 * Clears flags.
		 *
		 * \param[in] f the flags to disable.
		 */
		void unset_flags(unsigned int f) {
			flags &= ~f;
		}

	protected:
		/**
		 * A target position and an error.
		 */ 
		std::pair<Point,double> target_position;
		
		/**
		 * A target orientation and an error.
		 */
		std::pair<double,double> target_orientation;
		
		/*
		 * \c true if the robot should dribble when it has the ball, or \c false
		 * if not.
		 */
		bool need_dribble;
		
		/*
		 * The flags currently in force for this Navigator.
		 */
		unsigned int flags;
		

		
	private:
	
};


/**
 * Single instance of TeamNavigator handles navigation for all players on the
 * controllable team.
 */
class TeamNavigator : public ByRef, public sigc::trackable{
	public:
	
		/**
		 * A pointer to a TeamNavigator object.
		 */
		typedef RefPtr<TeamNavigator> Ptr;
	
		/**
		 * Constructs a new TeamNavigator.
		 *
		 * \param[in] world the World in which to navigate.
		 */
		TeamNavigator(World::Ptr world):the_world(world){
			the_world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &TeamNavigator::on_player_removed));
			the_world->friendly.signal_player_added.connect(sigc::mem_fun(this, &TeamNavigator::on_player_added));
		}
		
		/**
		 * A map from player to the player's navigator.
		 *
		 * The rest of the AI operates on the player's navigator, not the team
		 * navigator.
		 *
		 * \param[in] p the player whose navigator to fetch.
		 *
		 * \return the navigator for the player.
		 */
		Navigator::Ptr operator[](Player::Ptr p){
			return navis[p->address()];
		}
	
	
		virtual void tick()=0;
		
	protected:
		/**
		 * When a player is added will need to make a new player navigator for
		 * it.
		 */
		void on_player_added(unsigned int, Player::Ptr play){
			navis.insert(std::pair<uint64_t, Navigator::Ptr>(play->address(), create_navigator(play)));
		}
		
		/**
		 * When a player is removed will need to destroy player navigator for
		 * it.
		 */		
		void on_player_removed(unsigned int, Player::Ptr play){
			navis.erase(play->address());
		}
		
		virtual Navigator::Ptr create_navigator(Player::Ptr play)=0;
				
		const World::Ptr the_world;
		
		std::map<uint64_t, Navigator::Ptr> navis;	

};

#endif

