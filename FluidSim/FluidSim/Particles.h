#ifndef PARTICLES_H
#define PARTICLES_H

#include "../../../eigen-3.4.0/Eigen/Dense"
#include "Particle.h"
#include <unordered_map>
#include <vector>
#include <omp.h>

// Code is HEAVILY influenced by https://lucasschuermann.com/writing/implementing-sph-in-2d#citation

// Structures the particles into grids which can help speed up calculations
struct GridCell {
	std::vector<size_t> particleIndices; // Stores indices of particles in the cell
};

// Class representing the list of particles, as well as operations performed on them
class ParticleList {
public:
	std::unordered_map<int, GridCell> grid; // Spatial hash grid

	int computeGridIndex(const Eigen::Vector2d& pos) {
		int x = static_cast<int>(pos(0) / H);
		int y = static_cast<int>(pos(1) / H);
		return (x * 73856093) + (y * 19349663); // Unique hash for cell index
	}

	// Builds the particle grid
	void buildGrid() {
		grid.clear(); // Reset grid each frame
		for (size_t i = 0; i < m_particles.size(); ++i) {
			int cellIndex = computeGridIndex(m_particles[i].getPosition());
			grid[cellIndex].particleIndices.push_back(i);
		}
	}

	// Returns a vector of neighbors for a given position
	std::vector<int> getNeighborCells(const Eigen::Vector2d& pos) {
		int x = static_cast<int>(pos(0) / H);
		int y = static_cast<int>(pos(1) / H);
		std::vector<int> neighbors;

		for (int dx = -1; dx <= 1; ++dx) {
			for (int dy = -1; dy <= 1; ++dy) {
				neighbors.push_back(computeGridIndex(Eigen::Vector2d((x + dx) * H, (y + dy) * H)));
			}
		}
		return neighbors;
	}

	// Applies mouse drag to particles by adding force to them
	void applyMouseDragForce(double mouseX, double mouseY, const Eigen::Vector2d& force)
	{
		// Convert screen coordinates to simulation coordinates
		// We can ignore any x values less than WINDOW_WIDTH/4 or more than 3 * WINDOW_WIDTH/4 by clamping
		if (mouseX < WINDOW_WIDTH / 4.0f) {
			mouseX = WINDOW_WIDTH / 4.0f;
		}

		if (mouseX > 3.0f * WINDOW_WIDTH / 4.0f) {
			mouseX = 3.0f * WINDOW_WIDTH / 4.0f;
		}

		// 'Flip' y so that the bottom is closer to 0 and the top is closer to WINDOW_HEIGHT
		mouseY = WINDOW_HEIGHT - mouseY;

		if (mouseY > 3.0f * WINDOW_HEIGHT / 4.0f) {
			mouseY = 3.0f * WINDOW_HEIGHT / 4.0f;
		}

		if (mouseY < WINDOW_HEIGHT / 4.0f) {
			mouseY = WINDOW_HEIGHT / 4.0f;
		}

		mouseX -= WINDOW_WIDTH / 4.0f;
		mouseY -= WINDOW_HEIGHT / 4.0f;
	
		// Particle x values go from BOUNDARY to (VIEW_WIDTH - BOUNDARY)
		// Mouse x values go from 0 to WINDOW_WIDTH
		// Have to convert between the two for accurate results
		// Same goes for y values
		double worldMouseX = static_cast<float>(BOUNDARY + (mouseX / (WINDOW_WIDTH / 2.0f)) * (VIEW_WIDTH - 2.0f * BOUNDARY));
		double worldMouseY = static_cast<float>(BOUNDARY + (mouseY / (WINDOW_HEIGHT / 2.0f)) * (VIEW_HEIGHT - 2.0f * BOUNDARY));

		for (auto& p : m_particles)
		{
			// Compute distance from mouse to particle
			Eigen::Vector2d diff = p.getPosition() - Eigen::Vector2d(worldMouseX, worldMouseY);
			float distance = diff.norm();

			// Apply force if within a certain radius
			if (distance < 2 * H) // Adjust radius as needed, using 2 * H here
			{
				p.setForce(p.getForce() + force);
			}
		}
	}

	// Constructor
	ParticleList(){}

	// Getters/Setters
	std::vector<Particle> getParticles() { return m_particles; }
	void setParticles(std::vector<Particle> particles) { m_particles = particles; }
	std::vector<float> getParticlePositions() {
		std::vector<float> positions;
		for (auto& pi : m_particles) {
			positions.push_back(pi.getPosition()(0));
			positions.push_back(pi.getPosition()(1));
		}
		return positions;
	}

	// Clear all particles
	void clearParticles() { m_particles.clear(); }

	// Add a particle to the list
	void addParticle(Particle p) { m_particles.push_back(p); }

	// Returns the number of particles
	size_t size() { return m_particles.size(); }

	// Returns particle data
	Particle* data() { return m_particles.data(); }

	// Calculates densities using OpenMP for parallelism
	void calculateDensities()
	{
		#pragma omp parallel for
		for (size_t i = 0; i < m_particles.size(); ++i)
		{
			auto& pi = m_particles[i];
			pi.setRho(0.0f);

			// Get neighboring cells
			std::vector<int> neighborCells = getNeighborCells(pi.getPosition());

			// Iterate over neighbors
			for (int cellIndex : neighborCells) {
				auto it = grid.find(cellIndex);
				if (it == grid.end()) continue; // Skip empty cells
				for (size_t j : grid[cellIndex].particleIndices)
				{
					Eigen::Vector2d rij = m_particles[j].getPosition() - pi.getPosition();
					float r2 = rij.squaredNorm();

					if (r2 < HSQ) { // Use squared distance for efficiency
						pi.setRho(pi.getRho() + MASS * W_POLY6 * pow(HSQ - r2, 3.0f));
					}
				}
			}
			pi.setP(GAS_CONST * (pi.getRho() - REST_DENS)); // Equation 12
		}
	}
	
	// Calculates forces using OpenMP for parallelism
	void calculateForces()
	{
		#pragma omp parallel for
		for (size_t i = 0; i < m_particles.size(); ++i)
		{
			auto& pi = m_particles[i];
			Eigen::Vector2d pressure(0.f, 0.f);
			Eigen::Vector2d viscosity(0.f, 0.f);

			// Get neighboring cells
			std::vector<int> neighborCells = getNeighborCells(pi.getPosition());

			for (int cellIndex : neighborCells) {
				auto it = grid.find(cellIndex);
				if (it == grid.end()) continue; // Skip empty cells

				for (size_t j : it->second.particleIndices)
				{
					if (i == j) continue;

					Eigen::Vector2d rij = m_particles[j].getPosition() - pi.getPosition();
					float r2 = rij.squaredNorm();

					if (r2 < HSQ) { // Only compute sqrt if within influence radius
						float r = sqrt(r2); // Now only computed when necessary

						// pressure force
						pressure += -rij.normalized() * MASS * (pi.getP() + m_particles[j].getP()) /
							(2.0f * m_particles[j].getRho()) * W_SPIKY * pow(H - r, 3.f);

						// viscosity force
						viscosity += VISC * MASS * (m_particles[j].getVelocity() - pi.getVelocity()) /
							m_particles[j].getRho() * W_VISCOSITY * (H - r);
					}
				}
			}

			Eigen::Vector2d fgrav = G * MASS / pi.getRho();
			pi.setForce(pressure + viscosity + fgrav);
		}
	}

	// Integrating using Euler's method with OpenMP for parallelism
	void Integrate()
	{
		#pragma omp parallel for
		for (size_t i = 0; i < m_particles.size(); ++i)
		{
			auto& p = m_particles[i];
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