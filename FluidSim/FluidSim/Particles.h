#ifndef PARTICLES_H
#define PARTICLES_H

#include "../../../eigen-3.4.0/Eigen/Dense"
#include "Particle.h"
#include <vector>

// Code is HEAVILY influenced by https://lucasschuermann.com/writing/implementing-sph-in-2d#citation

class ParticleList {
public:
	ParticleList(){}
	std::vector<Particle> getParticles() { return m_particles; }
	void setParticles(std::vector<Particle> particles) { m_particles = particles; }
	void clearParticles() { m_particles.clear(); }
	void addParticle(Particle p) { m_particles.push_back(p); }
	size_t size() { return m_particles.size(); }
	Particle* data() { return m_particles.data(); }
	std::vector<float> getParticlePositions(){
		std::vector<float> positions;
		for (auto& pi : m_particles) {
			positions.push_back(pi.getPosition()(0));
			positions.push_back(pi.getPosition()(1));
		}
		return positions;
	}

	void calculateDensities()
	{
		for (auto& pi : m_particles)
		{
			pi.setRho(0.0f);
			for (auto& pj : m_particles)
			{
				Eigen::Vector2d rij = pj.getPosition() - pi.getPosition();
				float r = rij.norm();
				float r2 = rij.squaredNorm();

				if (r < H)
				{
					// this computation is symmetric
					pi.setRho(pi.getRho() + MASS * W_POLY6 * pow(HSQ - r2, 3.0f));
				}
			}
			pi.setP(GAS_CONST * (pi.getRho() - REST_DENS)); // Equation 12
		}
	}

	void calculateForces()
	{
		for (auto& pi : m_particles)
		{
			Eigen::Vector2d pressure(0.f, 0.f);
			Eigen::Vector2d viscosity(0.f, 0.f);
			for (auto& pj : m_particles)
			{
				if (&pi == &pj)
				{
					continue;
				}

				Eigen::Vector2d rij = pj.getPosition() - pi.getPosition();
				float r = rij.norm();

				if (r < H)
				{
					// compute pressure force contribution
					pressure += -rij.normalized() * MASS * (pi.getP() + pj.getP()) / 
						(2.0f * pj.getRho()) * W_SPIKY * pow(H - r, 3.f);
					// compute viscosity force contribution
					viscosity += VISC * MASS * (pj.getVelocity() - pi.getVelocity()) / pj.getRho() * W_VISCOSITY * (H - r);
				}
			}
			Eigen::Vector2d fgrav = G * MASS / pi.getRho();
			pi.setForce(pressure + viscosity + fgrav);
		}
	}

	void Integrate()
	{
		for (auto& p : m_particles)
		{
			// Leapfrog Integration
			p.setVelocity(p.getVelocity() + DT * p.getForce() / p.getRho());
			p.setPosition(p.getPosition() + DT * p.getVelocity());

			Eigen::Vector2d velocity = p.getVelocity();
			Eigen::Vector2d position = p.getPosition();

			// enforce boundary conditions
			if (p.getPosition()(0) - BOUNDARY < 0.f)
			{
				velocity(0) *= BOUND_DAMPING;
				position(0) = BOUNDARY;
			}
			if (p.getPosition()(0) + BOUNDARY > VIEW_WIDTH)
			{
				velocity(0) *= BOUND_DAMPING;
				position(0) = VIEW_WIDTH - BOUNDARY;
			}
			if (p.getPosition()(1) - BOUNDARY < 0.f)
			{
				velocity(1) *= BOUND_DAMPING;
				position(1) = BOUNDARY;
			}
			if (p.getPosition()(1) + BOUNDARY > VIEW_HEIGHT)
			{
				velocity(1) *= BOUND_DAMPING;
				position(1) = VIEW_HEIGHT - BOUNDARY;
			}

			p.setVelocity(velocity);
			p.setPosition(position);
		}
	}
private:
	std::vector<Particle> m_particles;
};

#endif