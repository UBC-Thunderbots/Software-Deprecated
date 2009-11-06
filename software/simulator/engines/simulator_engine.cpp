#include "simulator/engine.h"
#include "simulator/engines/ballODE.h"
#include "simulator/engines/playerODE.h"

#define MU 0.5		// the global mu to use
namespace {

	//
	// The limit of floating-point precision.
	//
	const double EPS = 1.0e-9;

	//
	// The maximum acceleration of a robot, in metres per second squared.
	//
	const double BOT_MAX_ACCELERATION = 10.0;

	//
	// The maximum velocity of a robot, in metres per second.
	//
	const double BOT_MAX_VELOCITY = 5.0;

	//
	// The acceleration due to friction against the ball, in metres per second squared.
	//
	const double BALL_DECELERATION = 6.0;

	//
	// The force of gravity N/kg
	//
	const double GRAVITY = -0.05;

			static dWorldID eworlds;
			static dSpaceID spaces;
			static dGeomID grounds;
			static dJointGroupID contactgroups;
static ballODE::ptr st_ball;

static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
  int i;

  // only collide things with the ground
  int g1 = (o1 == grounds);
  int g2 = (o2 == grounds);
  if ((g1 ^ g2)) {

  dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);

  dContact contact[3];		// up to 3 contacts per box
  for (i=0; i<3; i++) {
    contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
    contact[i].surface.mu = MU;
    contact[i].surface.soft_cfm = 0.01;
  }
  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
    for (i=0; i<numc; i++) {
      dJointID c = dJointCreateContact (eworlds,contactgroups,contact+i);
      dJointAttach (c,b1,b2);
    }
  }
}else {

  g1 = (o1 == st_ball->ballGeom);
   g2 = (o2 == st_ball->ballGeom);
if ((g1 ^ g2)){

dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);

  dContact contact[3];		// up to 3 contacts per box
  for (i=0; i<3; i++) {
    contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
    contact[i].surface.mu = MU;
    contact[i].surface.soft_cfm = 0.01;
  }
  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
    for (i=0; i<numc; i++) {
      dJointID c = dJointCreateContact (eworlds,contactgroups,contact+i);
      dJointAttach (c,b1,b2);
    }
  }

}


}


}
/*
static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{

  int i,n;

  // only collide things with the ground
  int g1 = (o1 == grounds);
  int g2 = (o2 == grounds);
  if (true) {

  const int N = 20;
  dContact contact[N];
  n = dCollide (o1,o2,N,&contact[0].geom,sizeof(dContact));
  if (n > 0) {
    for (i=0; i<n; i++) {


contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
	dContactSoftERP | dContactSoftCFM | dContactApprox1;
      contact[i].surface.mu = dInfinity;
      contact[i].surface.slip1 = 0.1;
      contact[i].surface.slip2 = 0.1;
      contact[i].surface.soft_erp = 0.5;
      contact[i].surface.soft_cfm = 0.3;

      dJointID c = dJointCreateContact (eworlds,contactgroups,&contact[i]);
      dJointAttach (c,
		    dGeomGetBody(contact[i].geom.g1),
		    dGeomGetBody(contact[i].geom.g2));
    }
  }
  }
}
*/


	//
	// A simulator_engine.
	//
	class sim_engine : public simulator_engine {
		public:
			dWorldID eworld;
			dSpaceID space;
			dGeomID ground;
			dJointGroupID contactgroup;

			sim_engine(){


				eworld = dWorldCreate(); 
				dWorldSetGravity (eworld,0,0.0,GRAVITY);

				space = dHashSpaceCreate (0);

  				ground = dCreatePlane (space,0,0,1,0.1);
    				dWorldSetContactSurfaceLayer(eworld, 0.01);
				contactgroup = dJointGroupCreate (0);

				ballODE::ptr b(new ballODE(eworld, space));
				the_ball = b;
st_ball = the_ball;

				eworlds= eworld;
				spaces= space;
				grounds= ground;
				contactgroups= contactgroup;
 dWorldSetLinearDamping (eworld, 0.9);


dWorldSetCFM (eworld, 0.2);


 				dInitODE2(0);
			}




			void tick() {
 				dSpaceCollide (space,0,&nearCallback);
				
				dWorldStep (eworld, 5);
				 dJointGroupEmpty (contactgroup);
			}
			void setWorld(dWorldID world) {
				eworld = world;
			}
			ball_impl::ptr get_ball() {
				
				return the_ball;
			}

			playerODE::ptr add_player() {
				playerODE::ptr p(new playerODE(eworld, space));
				the_players.push_back(p);
				return p;
			}

			void remove_player(player_impl::ptr p) {
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (playerODE::ptr::cast_static(the_players[i]) == p) {
						the_players.erase(the_players.begin() + i);
						return;
					}
				}
			}

			Gtk::Widget *get_ui_controls() {
				return 0;
			}

			simulator_engine_factory &get_factory();

		private:
			ballODE::ptr the_ball;
			std::vector<playerODE::ptr> the_players;
	};

	//
	// A factory for creating sim_engines.
	//
	class sim_engine_factory : public simulator_engine_factory {
		public:
			sim_engine_factory() : simulator_engine_factory("Open Dynamics Engine Simulator") {
			}

			simulator_engine::ptr create_engine(xmlpp::Element *) {
				simulator_engine::ptr p(new sim_engine);
				return p;
			}
	};

	//
	// The global instance of sim_engine_factory.
	//
	sim_engine_factory fact;

	simulator_engine_factory &sim_engine::get_factory() {
		return fact;
	}
}

