#ifndef PARTICLE_H
#define PARTICLE_H

#include "../../../eigen-3.4.0/Eigen/Dense"

// Class representing a single particle
class Particle
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	// Constructor
	Particle(float x, float y) {
		m_position = Eigen::Vector2d(x, y); // position
		m_velocity = Eigen::Vector2d(0.0f, 0.0f); // velocity
		m_force = Eigen::Vector2d(0.0f, 0.0f); // force
		m_rho = 0.0f; // density
		m_p = 0.0f; // pressure
	}
	
	// Getters/Setters
	Eigen::Vector2d getPosition() { return m_position; }
	Eigen::Vector2d getVelocity() { return m_velocity; }
	Eigen::Vector2d getForce() { return m_force; }
	float getRho() { return m_rho; }
	float getP() { return m_p; }
	void setPosition(Eigen::Vector2d position) { m_position = position; }
	void setVelocity(Eigen::Vector2d velocity) { m_velocity = velocity; }
	void setForce(Eigen::Vector2d force) { m_force = force; }
	void setRho(float rho) { m_rho = rho; }
	void setP(float p) { m_p = p; }

private:
	Eigen::Vector2d m_position, m_velocity, m_force;
	float m_rho, m_p;
};

#endif