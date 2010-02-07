#include "simulator/ball.h"
#include "simulator/field.h"
#include <ode/ode.h>

//
// The back-end behind an ODE ball object.
// 
//
class ballODE : public simulator_ball_impl {
	public:

			typedef Glib::RefPtr<ballODE> ptr;
			dWorldID world;
			dBodyID body;
			dGeomID ballGeom;
			dMass m;
			
			
			ballODE(dWorldID dworld, dSpaceID dspace, double radius   = 0.042672/2, double mass = 0.046);
			
			/*ballODE(dWorldID dworld, dSpaceID dspace, double radius);
			
			ballODE(dWorldID dworld, dSpaceID dspace);
			*/
			~ballODE();

			point position() const;

			point velocity() const;

			point acceleration() const;
			double get_height() const;

			double getRadius();

			void ext_drag(const point &pos, const point &vel);

			bool in_goal();

	private:
			point the_position, the_velocity;
			 double dradius;
			 field::ptr fld;
			

};


