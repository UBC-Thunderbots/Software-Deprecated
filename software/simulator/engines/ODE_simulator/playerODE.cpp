#include "playerODE.h"
#include "world/timestep.h"
#include <iostream>
#include <math.h>
#include <algorithm>

namespace {

	double orientationFromMatrix(const dReal *t){
		return atan2(-t[1], t[0]);
	}
	
	//
	// The maximum acceleration of a robot, in metres per second squared.
	//
	const double BOT_MAX_ACCELERATION = 10.0;

	//
	// The maximum velocity of a robot, in metres per second.
	//
	const double BOT_MAX_VELOCITY = 5.0;
	
	//
	// The maximum angular velocity of a robot in radians per second
	//
	const double BOT_MAX_A_VELOCITY = 2.0;
	
	//
	// The maximum angular acceleration of a robot, in radians per second squared
	//
	const double BOT_MAX_A_ACCELERATION = 1.0;

	//
	// The acceleration due to friction against the ball, in metres per second squared.
	//
	const double BALL_DECELERATION = 6.0;

}

playerODE::playerODE (dWorldID eworld, dSpaceID dspace, dGeomID ballGeomi, double ups_per_tick) : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0), target_avelocity(0.0) {

	updates_per_tick = ups_per_tick;
	double dribble_radius = 0.005;//half a cm
	ballGeom = ballGeomi;
	double ballradius = dGeomSphereGetRadius(ballGeom);

	world = eworld;
	body = dBodyCreate(world);
	//body2 = dBodyCreate(world);
	double x_pos = 0.0;
	double y_pos = 0.0;
	jerkLimit = 30000.0;

	fcex=0;
	fcey=0;
	torquez=0;

	x_len = 0.18;
	y_len = 0.18;

	//dBodySetPosition(body, x_pos, y_pos, 0.0006);
	dBodySetPosition(body, x_pos, y_pos, 0.0515);
	//dGeomID robotGeom = dCreateBox (0,x_len,y_len,0.001);//10cm 
	dGeomID robotGeomTop = dCreateBox (0,x_len,y_len,0.1);

	double arm_width = 0.001;
	double arm_height = 0.01;

	dGeomID dribbleArmL = dCreateBox (0,dribble_radius*2.5,arm_width,arm_height);
	dGeomID dribbleArmR = dCreateBox (0,dribble_radius*2.5,arm_width,arm_height);

	//dMassSetCylinderTotal (&mass,0.1, 3,9.0/100,15.0/100);
	dMassSetCylinderTotal (&mass,4.0, 3,9.0/100,15.0/100);
	//dBodySetMass (body,&mass);
	//dBodySetLinearDamping (body, 0.12);
	dBodySetMass (body,&mass);
	momentInertia = 0.00405;
	//dGeomSetBody (robotGeom,body);
	dGeomSetBody (robotGeomTop,body);

	double arm_h_offset = ballradius - 0.051;

	dGeomSetBody (dribbleArmL,body);
	dGeomSetBody (dribbleArmR,body);

	dGeomSetOffsetPosition (dribbleArmL, x_len/2, y_len/2 + arm_width/2, arm_h_offset);
	dGeomSetOffsetPosition (dribbleArmR, -x_len/2, -y_len/2 - arm_width/2, arm_h_offset);

	//dSpaceAdd (dspace, robotGeom);
	dSpaceAdd (dspace, robotGeomTop);
	//dBodySetLinearDamping (body, 0.05);
	//dBodySetLinearDamping (body2, 0.05);
	//dBodySetAngularDamping (body, 0.12);
	//contactgroup = dJointGroupCreate (0);
	//createJointBetweenB1B2();
}

playerODE::~playerODE () {
	//dJointGroupDestroy (contactgroup);
	dBodyDestroy (body);
	//dBodyDestroy (body2);
}

void playerODE::createJointBetweenB1B2(){
	//dJointGroupDestroy (contactgroup);
	//contactgroup = dJointGroupCreate(0);
	//hinge=dJointCreateFixed (world, contactgroup);
	//const dReal *t = dBodyGetPosition (body);
//	double x = t[0];
//	double y = t[1];
	//double z = t[2];
	//z+=0.0005;  
	//dJointSetBallAnchor(hinge, x, y , z);
	//dJointAttach (hinge, body, body2);
	//dJointEnable (hinge); 
}

point playerODE::position() const {
	const dReal *t = dBodyGetPosition (body);
	return point(t[0], t[1]);
}

double playerODE::orientation() const {
	return orientationFromMatrix(dBodyGetRotation(body));
}

bool playerODE::has_ball() const {

	bool hasTheBall = true;
	double hasBallTolerance = 0.0025;
	const dReal *b = dBodyGetPosition (dGeomGetBody(ballGeom)); 
	const dReal *p = dBodyGetPosition (body);

	point ball_loc(b[0], b[1]);
	point play_loc(p[0], p[1]);
	point play_ball_diff = ball_loc - play_loc;
	point rel_play_ball_diff = play_ball_diff.rotate(-orientation());
	play_ball_diff  = rel_play_ball_diff;

	if(play_ball_diff.x < x_len/2 + dGeomSphereGetRadius(ballGeom) - hasBallTolerance){
		hasTheBall=false;
	}

	if(play_ball_diff.x > x_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
		hasTheBall=false;
	}
	if(play_ball_diff.y > y_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
		hasTheBall=false;
	}
	if(rel_play_ball_diff.x <0){
		hasTheBall=false;
	}
	double mag_y = abs(rel_play_ball_diff.y);
	double mag_x = abs(rel_play_ball_diff.x);

	if( mag_y/mag_x > y_len/x_len){
		hasTheBall=false;
	}

	return hasTheBall;
}

bool playerODE::has_ball(double tolerance){

	bool hasTheBall = true;
	double hasBallTolerance = tolerance;
	const dReal *b = dBodyGetPosition (dGeomGetBody(ballGeom)); 
	const dReal *p = dBodyGetPosition (body);

	point ball_loc(b[0], b[1]);
	point play_loc(p[0], p[1]);
	point play_ball_diff = ball_loc - play_loc;
	point rel_play_ball_diff = play_ball_diff.rotate(-orientation());
	play_ball_diff  = rel_play_ball_diff;

	if(play_ball_diff.x < x_len/2 + dGeomSphereGetRadius(ballGeom) - hasBallTolerance){
		hasTheBall=false;
	}

	if(play_ball_diff.x > x_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
		hasTheBall=false;
	}
	if(play_ball_diff.y > y_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
		hasTheBall=false;
	}
	if(rel_play_ball_diff.x <0){
		hasTheBall=false;
	}
	double mag_y = abs(rel_play_ball_diff.y);
	double mag_x = abs(rel_play_ball_diff.x);

	if( mag_y/mag_x > y_len/x_len){
		hasTheBall=false;
	}

	return hasTheBall;
}


bool playerODE::robot_contains_shape(dGeomID geom){
	dBodyID b = dGeomGetBody(geom);
	return (b==body);
}

void playerODE::pre_tic(){

	if(!posSet){
	
		const dReal *cur_vel = dBodyGetLinearVel(body);
		the_velocity.x = cur_vel[0];
		the_velocity.y = cur_vel[1];
		
		const dReal * t =  dBodyGetAngularVel (body);
		avelocity = t[2];
		
		point fce = (target_velocity-the_velocity)/BOT_MAX_VELOCITY*BOT_MAX_ACCELERATION*mass.mass;
		
		double torque = (target_avelocity-avelocity)/BOT_MAX_A_VELOCITY*BOT_MAX_A_ACCELERATION*momentInertia;		
		
		fcex = fce.x;
		fcey = fce.y;
		torquez=torque;
		
		dBodyEnable (body);
		dBodySetDynamic (body);
		
		dBodyAddTorque (body, 0.0, 0.0, torquez);
		dBodyAddForce (body, fcex, fcey, 0.0);

	}
	posSet=false;
}

void playerODE::move_impl(const point &vel, double avel) {					
		if(vel.len() > BOT_MAX_VELOCITY)
			target_velocity = vel/vel.len()*BOT_MAX_VELOCITY;
		else
			target_velocity=vel;
		    
		if(fabs(avel) > BOT_MAX_A_VELOCITY)
			target_avelocity = avel/fabs(avel)*BOT_MAX_A_VELOCITY;		
		else
			target_avelocity = avel;	
			
		target_velocity = target_velocity.rotate(orientation());	
}

void playerODE::dribble(double speed) {

	double max_Angular_vel = 5.0;

	if(speed<0 || speed>1)return;

	double maxTorque = 0.00001;//static_cast<double>(TIMESTEPS_PER_SECOND); //is this realistic???			
	double appliedTorque = -(speed*maxTorque);

	const dReal * t = dBodyGetAngularVel (dGeomGetBody(ballGeom));
	//std::cout<<"dribble"<< t[0]<<" "<<t[1]<<" "<<t[2]<<std::endl;

	//std::cout<<"dribble speed: "<<speed<<std::endl;
	point torqueAxis(0,1);
	torqueAxis = torqueAxis.rotate(orientation());

	torqueAxis*=appliedTorque;

	if(has_ball(0.012)){


		point ball_turn;
		ball_turn.x = t[0];
		ball_turn.y = t[1];
		if(! (ball_turn.len() > max_Angular_vel)){
			double forceMax = 0.0001;
			//std::cout<<"dribble"<<speed<<std::endl;
			//std::cout<<"dribble"<< t[0]<<" "<<t[1]<<" "<<t[2]<<std::endl;
			//dBodyAddTorque(dGeomGetBody(ballGeom), torqueAxis.x, torqueAxis.y, 0.0);
			point directionp(1,0);
			directionp = directionp.rotate(orientation());
			directionp = -directionp*forceMax*speed;
			dBodyAddForce(dGeomGetBody(ballGeom), directionp.x, directionp.y, 0.0);

		}

	}
}

void playerODE::kick(double strength) {

	if(strength <0 || strength >1)return;

	double maximum_impulse = 0.0001;
	point direction(1.0, 0.0);
	direction = direction.rotate(orientation());
	point impulse = strength*maximum_impulse*direction;

	if(has_ball(0.005)){
		dVector3 force;
		dWorldImpulseToForce (world, 1.0/static_cast<double>(TIMESTEPS_PER_SECOND),
				impulse.x, impulse.y,0.0, force);
		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}
}

void playerODE::chip(double strength) {

	if(strength <0 || strength >1)return;
	//std::cout<<"chip strength: "<<strength<<std::endl;
	double maximum_impulse = 0.0001;

	point direction(1.0/sqrt(2.0), 0.0);
	direction = direction.rotate(orientation());
	point impulse = strength*maximum_impulse*direction;

	double zimpulse = strength*maximum_impulse/sqrt(2.0);

	if(has_ball(0.01)){
		dVector3 force;
		dWorldImpulseToForce (world, 1.0/static_cast<double>(TIMESTEPS_PER_SECOND),
				impulse.x, impulse.y,zimpulse, force);
		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}
	//std::cout<<"chip strength: "<<strength<<std::endl;
}

void playerODE::ext_drag(const point &pos, const point &vel) {
	posSet = true;
	const dReal *t = dBodyGetPosition (body);
	//const dReal *t2 = dBodyGetPosition (body2);
	dBodySetPosition(body, pos.x, pos.y, t[2]);
	//dBodySetPosition(body2, pos.x, pos.y, t2[2]);
	dBodySetLinearVel(body,vel.x,vel.y,0.0);
	//dBodySetLinearVel(body2,vel.x,vel.y,0.0);
	dBodySetAngularVel (body, 0.0, 0.0, 0.0);
	//dBodySetAngularVel (body2, 0.0, 0.0, 0.0);
	//createJointBetweenB1B2();
	ext_drag_postprocess();
}

void playerODE::ext_rotate(double orient, double avel) {
	posSet=true;
	//#warning IMPLEMENT
	dMatrix3 RotationMatrix;
	dRFromAxisAndAngle (RotationMatrix, 0.0, 0.0, 1.0, orient);
	dBodySetRotation (body, RotationMatrix);
	//dBodySetRotation (body2, RotationMatrix);
	dBodySetAngularVel (body, 0.0, 0.0, avel);
	//dBodySetAngularVel (body2, 0.0, 0.0, avel);
	ext_rotate_postprocess();
}

