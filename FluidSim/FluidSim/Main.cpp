#include <iostream>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "../../../eigen-3.4.0/Eigen/Dense"
#include "Constants.h"
#include "Particles.h"
#include <vector>
#include <windows.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderLoop();
void initGLFW();
void endGLFW();
void initSPH();
void update();

// Used to track mouse dragging
double pressMouseX, pressMouseY, dragX, dragY;
bool mousePressed = false;
bool mouseReleased = false;

GLFWwindow* window;
Shader* shader;
unsigned int VAO, VBO, EBO;

// solver data
ParticleList particles;

// Ensures GPU usage
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main() {
	initGLFW();
	endGLFW();

	return 0;
}

// Initializes SPH by spwaning in the particles, code was modified from Lucas-Schuermann
void initSPH(void)
{
	for (float y = BOUNDARY + 8 * H; y < VIEW_HEIGHT - BOUNDARY * 2.f; y += H)
	{
		for (float x = VIEW_WIDTH / 4; x <= VIEW_WIDTH / 2; x += H)
		{
			if (particles.size() < DAM_PARTICLES)
			{
				float jitter = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
				particles.addParticle(Particle(x + jitter, y));
			}
			else
			{
				return;
			}
		}
	}
}

void update()
{
	// Continue with simulation steps
	particles.buildGrid();
	particles.calculateDensities();
	particles.calculateForces();

	// Make sure this is done AFTER calculate forces, otherwise it will get overriden
	if (mouseReleased) {
		mouseReleased = false;
		Eigen::Vector2d dragForce(dragX * 1000.0f, -dragY * 1000.0f);

		// Apply force to nearby particles
		particles.applyMouseDragForce(pressMouseX, pressMouseY, dragForce);
	}

	particles.Integrate();

	glBindVertexArray(VAO);
	std::vector<float> particlePositions = particles.getParticlePositions();
	glBufferData(GL_ARRAY_BUFFER, particlePositions.size() * sizeof(float), particlePositions.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
}

void initGLFW() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Fluid Simulation", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Shader Nonsense
	// /////////////////////////////////////////
	shader = new Shader("vertex.vert", "fragment.frag");
	// ///////////////////////////////////////////////////////////////////

	initSPH();


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	std::vector<float> particlePositions = particles.getParticlePositions();
	glBufferData(GL_ARRAY_BUFFER, particlePositions.size() * sizeof(float), particlePositions.data(), GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
		(void*)0);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glPointSize(4.0f);
	
	renderLoop();
}

void endGLFW() {
	if (shader != nullptr) {
		delete shader;
	}
	glfwTerminate();
}

void renderLoop() {
	// Render Loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		glClear(GL_COLOR_BUFFER_BIT);
		update();

		int vertexWindowWidthLocation = glGetUniformLocation(shader->ID, "windowWidth");
		int vertexWindowHeightLocation = glGetUniformLocation(shader->ID, "windowHeight");

		shader->use();

		glUniform1i(vertexWindowWidthLocation, WINDOW_WIDTH);
		glUniform1i(vertexWindowHeightLocation, WINDOW_HEIGHT);
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, particles.size());

		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		particles.clearParticles();
		initSPH();
	}

	// Detect mouse press and release
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mousePressed) {
		mousePressed = true;
		glfwGetCursorPos(window, &pressMouseX, &pressMouseY);
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && mousePressed) {
		mousePressed = false;
		mouseReleased = true;

		double releaseX, releaseY;
		glfwGetCursorPos(window, &releaseX, &releaseY);

		// Compute drag force vector from press to release
		dragX = releaseX - pressMouseX;
		dragY = releaseY - pressMouseY;
	}
}
