#ifndef CONSTANTS_H
#define CONSTANTS_H

# define M_PI           3.14159265358979323846
#include "../../../eigen-3.4.0/Eigen/Dense"

// Constants for 2D sph borrowed from https://lucasschuermann.com/writing/implementing-sph-in-2d#citation

const static Eigen::Vector2d G(0.f, -9.81f);   // external (gravitational) forces
const static float REST_DENS = 300.f;  // Rest Density used in Eq. 12
const static float GAS_CONST = 2000.0f; // const for equation of state
const static float H = 16.0f;		   // kernel radius
const static float HSQ = H * H;		   // radius^2 for optimization
const static float MASS = 2.5f;	   // assume all particles have the same mass
const static float VISC = 200.0f;	   // viscosity constant
const static float DT = 0.0007f;	   // integration timestep


const static float W_POLY6 = 4.f / (M_PI * pow(H, 8.f)); // Eq. 20
const static float W_SPIKY = -10.f / (M_PI * pow(H, 5.f)); // Eq. 21
const static float W_VISCOSITY = 40.f / (M_PI * pow(H, 5.f)); // Eq. 22

// simulation parameters
const static float BOUNDARY = H; // boundary epsilon
const static float BOUND_DAMPING = -0.5f;

// interaction
const static int DAM_PARTICLES = 200;

const static int WINDOW_WIDTH = 800;
const static int WINDOW_HEIGHT = 600;
const static double VIEW_WIDTH = 1.5 * 800.f;
const static double VIEW_HEIGHT = 1.5 * 600.f;

#endif