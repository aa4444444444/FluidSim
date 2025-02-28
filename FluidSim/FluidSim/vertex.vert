#version 330 core
layout (location = 0) in vec2 aPos;
uniform int windowWidth;
uniform int windowHeight;
out vec3 ourColor;
void main()
{
	float xPos = (aPos.x - windowWidth / 2.0f) / windowWidth;
	float yPos = (aPos.y - windowHeight / 2.0f) / windowHeight;
	gl_Position = vec4(xPos, yPos, 1.0, 1.0);
}