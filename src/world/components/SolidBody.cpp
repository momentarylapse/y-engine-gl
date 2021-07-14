/*
 * SolidBodyComponent.cpp
 *
 *  Created on: Jul 13, 2021
 *      Author: michi
 */

#include "SolidBody.h"
#include "Collider.h"
#include "../World.h"
#include "../Object.h"
#include "../Model.h"
#include "../../y/EngineData.h"
#include "../../lib/file/msg.h"

const kaba::Class *SolidBody::_class = nullptr;


#if HAS_LIB_BULLET
#include <btBulletDynamicsCommon.h>


btVector3 bt_set_v(const vector &v);
btQuaternion bt_set_q(const quaternion &q);
vector bt_get_v(const btVector3 &v);
btTransform bt_set_trafo(const vector &p, const quaternion &q);
#endif


static int num_insane=0;


inline bool ainf_v(vector &v) {
	if (inf_v(v))
		return true;
	return (v.length_fuzzy() > 100000000000.0f);
}

inline bool TestVectorSanity(vector &v,char *name) {
	if (ainf_v(v)) {
		num_insane++;
		v=v_0;
		if (num_insane>100)
			return false;
		msg_error(format("Vektor %s unendlich!!!!!!!",name));
		return true;
	}
	return false;
}




#define _realistic_calculation_

#define VelThreshold			1.0f
#define AccThreshold			10.0f
#define MaxTimeTillFreeze		1.0f

#define unfreeze(object)	(object)->time_till_freeze = MaxTimeTillFreeze; (object)->frozen = false



SolidBody::SolidBody(Model *o) {
	msg_write("SOLID BODY");
	active = false;
	passive = false;
	mass = 0;
	mass_inv = 0;
	body = nullptr;

	vel = rot = v_0;
	rotating  = true;
	moved = false;
	frozen = false;
	time_till_freeze = 0;

	vel = rot = v_0;
	vel_surf = acc = v_0;

	force_int = torque_int = v_0;
	force_ext = torque_ext = v_0;

	g_factor = 1;
	test_collisions = true;



	// import
	active = o->physics_data_.active;
	passive = o->physics_data_.passive;
	test_collisions = o->physics_data_.test_collisions;
	mass = o->physics_data_.mass;
	theta_0 = o->physics_data_.theta_0;

#if HAS_LIB_BULLET
	btCollisionShape *col_shape = nullptr;
	auto col = (Collider*)o->get_component(Collider::_class);
	if (col)
		col_shape = col->col_shape;


	btTransform start_transform = bt_set_trafo(o->pos, o->ang);

	btScalar _mass(active ? mass : 0);
	btVector3 local_inertia(0, 0, 0);
	//if (isDynamic)
	if (col_shape) {
		col_shape->calculateLocalInertia(_mass, local_inertia);
		theta_0._00 = local_inertia.x();
		theta_0._11 = local_inertia.y();
		theta_0._22 = local_inertia.z();
	}

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* motion_state = new btDefaultMotionState(start_transform);
	btRigidBody::btRigidBodyConstructionInfo rb_info(_mass, motion_state, col_shape, local_inertia);
	body = new btRigidBody(rb_info);

	body->setUserPointer(this);
	update_mass();

	//else if (o->test_collisions)
	//	dynamicsWorld->addCollisionObject(o->body);
#endif
}

SolidBody::~SolidBody() {
#if HAS_LIB_BULLET
	delete body->getMotionState();
	delete body;
#endif
}

void SolidBody::on_init() {
	if (active)
		mass_inv = 1.0f / mass;
	else
		mass_inv = 0;
	theta = theta_0;
	update_theta();
}




void SolidBody::add_force(const vector &f, const vector &rho) {
	if (engine.elapsed<=0)
		return;
	if (!active)
		return;
	if (world.physics_mode == PhysicsMode::FULL_EXTERNAL) {
#if HAS_LIB_BULLET
		body->activate(); // why doesn't this happen automatically?!? bug in bullet?
		body->applyForce(bt_set_v(f), bt_set_v(rho));
#endif
	} else {
		force_ext += f;
		torque_ext += vector::cross(rho, f);
		//TestVectorSanity(f, "f addf");
		//TestVectorSanity(rho, "rho addf");
		//TestVectorSanity(torque, "Torque addf");
		unfreeze(this);
	}
}

void SolidBody::add_impulse(const vector &p, const vector &rho) {
	if (engine.elapsed<=0)
		return;
	if (!active)
		return;
	if (world.physics_mode == PhysicsMode::FULL_EXTERNAL) {
#if HAS_LIB_BULLET
		body->activate();
		body->applyImpulse(bt_set_v(p), bt_set_v(rho));
#endif
	} else {
		vel += p / mass;
		//rot += ...;
		unfreeze(this);
	}
}

void SolidBody::add_torque(const vector &t) {
	if (engine.elapsed <= 0)
		return;
	if (!active)
		return;
	if (world.physics_mode == PhysicsMode::FULL_EXTERNAL) {
#if HAS_LIB_BULLET
		body->activate();
		body->applyTorque(bt_set_v(t));
#endif
	} else {
		torque_ext += t;
		//TestVectorSanity(Torque,"Torque addt");
		unfreeze(this);
	}
}

void SolidBody::add_torque_impulse(const vector &l) {
	if (engine.elapsed <= 0)
		return;
	if (!active)
		return;
	if (world.physics_mode == PhysicsMode::FULL_EXTERNAL) {
#if HAS_LIB_BULLET
		body->activate();
		body->applyTorqueImpulse(bt_set_v(l));
#endif
	} else {
		//rot += ...
		//TestVectorSanity(Torque,"Torque addt");
		unfreeze(this);
	}
}



void SolidBody::do_physics(float dt) {
	if (dt <= 0)
		return;

	auto o = get_owner<Object>();


	if (_vec_length_fuzzy_(force_int) * mass_inv > AccThreshold)
	{unfreeze(this);}

	if (active and !frozen) {

		if (inf_v(o->pos))	msg_error("inf   CalcMove Pos  1");
		if (inf_v(vel))	msg_error("inf   CalcMove Vel  1");

			// linear acceleration
			acc = force_int * mass_inv;

			// integrate the equations of motion.... "euler method"
			vel += acc * dt;
			o->pos += vel * dt;

		if (inf_v(acc))	msg_error("inf   CalcMove Acc");
		if (inf_v(vel))	msg_error("inf   CalcMove Vel  2");
		if (inf_v(o->pos))	msg_error("inf   CalcMove Pos  2");

		//}

		// rotation
		if ((rot != v_0) or (torque_int != v_0)) {

			quaternion q_dot, q_w;
			q_w = quaternion( 0, rot );
			q_dot = 0.5f * q_w * o->ang;
			o->ang += q_dot * dt;
			o->ang.normalize();

			#ifdef _realistic_calculation_
				vector L = theta * rot + torque_int * dt;
				update_theta();
				rot = theta_inv * L;
			#else
				UpdateTheta();
				rot += theta_inv * torque_int * dt;
			#endif
		}
	}

	// new orientation
	o->update_matrix();
	update_theta();

	o->_ResetPhysAbsolute_();

	// reset forces
	force_int = torque_int = v_0;

	// did anything change?
	moved = false;
	//if ((Pos!=Pos_old)or(ang!=ang_old))
	//if ( (vel_surf!=v_0) or (VecLengthFuzzy(Pos-Pos_old)>2.0f*Elapsed) )//or(VecAng!=ang_old))
	if (active) {
		if ( (vel_surf != v_0) or (_vec_length_fuzzy_(vel) > VelThreshold) or (_vec_length_fuzzy_(rot) * o->prop.radius > VelThreshold))
			moved = true;
	} else {
		frozen = true;
	}
	// would be best to check on the sub models....
	/*if (model)
		if (model->bone.num > 0)
			moved = true;*/

	if (moved) {
		unfreeze(this);
	} else if (!frozen) {
		time_till_freeze -= dt;
		if (time_till_freeze < 0) {
			frozen = true;
			force_ext = torque_ext = v_0;
		}
	}
}



// rotate inertia tensor into world coordinates
void SolidBody::update_theta() {
	if (active) {
		auto r = matrix3::rotation_q(get_owner<Model>()->ang);
		auto r_inv = r.transpose();
		theta = (r * theta_0 * r_inv);
		theta_inv = theta.inverse();
	} else {
		// Theta and ThetaInv already = identity
		theta_inv = matrix3::ZERO;
	}
}



// scripts have to call this after
void SolidBody::update_data() {
	unfreeze(this);
	if (!active) {
		get_owner<Object>()->update_matrix();
		update_theta();
	}

	// set ode data..
}


void SolidBody::update_motion() {
#if HAS_LIB_BULLET
	btTransform trans;
	body->setLinearVelocity(bt_set_v(vel));
	body->setAngularVelocity(bt_set_v(rot));
	auto o = get_owner<Model>();
	trans.setRotation(bt_set_q(o->ang));
	trans.setOrigin(bt_set_v(o->pos));
	body->getMotionState()->setWorldTransform(trans);
#endif
}

void SolidBody::update_mass() {
#if HAS_LIB_BULLET
	if (active) {
		btScalar mass(active);
		btVector3 localInertia(theta_0._00, theta_0._11, theta_0._22);
		//if (colShape)
		//	colShape->calculateLocalInertia(mass, localInertia);
		body->setMassProps(mass, localInertia);
	} else {
		btScalar mass(0);
		btVector3 localInertia(0, 0, 0);
		//body->setMassProps(mass, localInertia);
	}
#endif
}