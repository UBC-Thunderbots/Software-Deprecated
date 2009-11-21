#include "world/player_impl.h"
#include <ode/ode.h>



//
// The back-end behind an ODE player object.
// 
//
class playerODE : public player_impl {
	public:
	
	double x_len;
	double y_len;
	
	typedef Glib::RefPtr<playerODE> ptr;
	//The world constructed by the simulatiuon engine
	private:
	
	dWorldID world;
	dBodyID body;
	dMass mass;
	dMass mass2;
	bool posSet;
	dBodyID body2;
	dJointGroupID contactgroup;
	dJointID hinge;
	point the_position, the_velocity, target_velocity;
	double the_orientation, avelocity, target_avelocity;
	dGeomID ballGeom;
	public:

	playerODE( dWorldID dworld, dSpaceID dspace,  dGeomID ballGeom);
	~playerODE();
//void tick();

void update();

	point position() const ;

			double orientation() const ;

			bool has_ball() const ;
		//
		//
		//
		bool robot_contains_shape(dGeomID geom);
				
			
protected:
			void move_impl(const point &vel, double avel) ;
			
public:
			void dribble(double speed) ;

			void kick(double strength) ;

			void chip(double strength) ;

			void ui_set_position(const point &pos);
			
			void createJointBetweenB1B2();
};



