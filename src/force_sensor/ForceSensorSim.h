// ForceSensorSim.h
// Force sensor for SAI2-Simulation
#ifndef FORCE_SENSOR_SIM_H
#define FORCE_SENSOR_SIM_H

#include <Sai2Model.h>
#include <Sai2Simulation.h>
#include <Eigen/Dense>
#include <string>
#include <vector>
#include "filters/ButterworthFilter.h"

// Basic data structure for force sensor data
struct ForceSensorData {
public:
	// TODO: should probably add some sensor identity as well
	std::string _robot_name; // name of robot to which sensor is attached
	std::string _link_name; // name of link to which sensor is attached
	// transform from link to sensor frame. Measured moments are with respect to
	// the sensor frame origin
	Eigen::Affine3d _transform_in_link;
	Eigen::Vector3d _force; // in sensor frame
	Eigen::Vector3d _moment; // in sensor frame

public:
	// ctor: assign defaults
	ForceSensorData()
	: _robot_name(""),
	_link_name(""),
	_transform_in_link(Eigen::Affine3d::Identity()),
	_force(Eigen::Vector3d::Zero()),
	_moment(Eigen::Vector3d::Zero())
	{
		// nothing to do
	}
};

// Simulated force sensor type.
// Note that this implementation ignores the mass and inertia of the object
// attached beyond the force sensor.
// Note also that this assumes the force sensor to be located just between the
// link and the end-effector. That is, it cannot account for any joint reaction
// forces in case the sensor is to be attached on the link between two joints.
class ForceSensorSim {
public:
	// ctor
	ForceSensorSim(
		const std::string& robot_name,
		const std::string& link_name,
		const Eigen::Affine3d& transform_in_link,
		Sai2Model::Sai2Model* model);

	//dtor
	~ForceSensorSim();

	// update force information
	void update(Simulation::Sai2Simulation* sim);

	// get force applied to sensor body in world coordinates
	void getForce(Eigen::Vector3d& ret_force);

	// get force applied to sensor body in local sensor frame
	void getForceLocalFrame(Eigen::Vector3d& ret_force);

	// get moment applied to sensor body in world coordinates
	void getMoment(Eigen::Vector3d& ret_moment);

	// get moment applied to sensor body in local sensor frame
	void getMomentLocalFrame(Eigen::Vector3d& ret_moment);

	// Apply Butterworth filter to the force data
	// get force and moment applied to sensor body in world frame as a 6dof vector (force first moment second)
	void getForceMoment(Eigen::VectorXd& ret_force_moment);

	// get force and moment applied to sensor body in local sensor frame as a 6dof vector (force first moment second)
	void getForceMomentLocalFrame(Eigen::VectorXd& ret_force_moment);

	void enableFilter(const double fc);

	// Discretly remove spikes from the force data
	void removeSpike(const double force_threshold);

public:
	// handle to model interface
	Sai2Model::Sai2Model* _model;

	// last updated data
	ForceSensorData* _data;

	// filter
	ButterworthFilter* _force_filter;
	ButterworthFilter* _moment_filter;
	bool _filter_on;

	//spike removal
	bool _remove_spike;
	bool _first_iteration;
	double _force_threshold;
	Eigen::Vector3d _previous_force;
	Eigen::Vector3d _previous_torque;

};

#endif //FORCE_SENSOR_SIM_H
