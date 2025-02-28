#version 330 core
out vec4 FragColor;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
	FragColor = vec4(rand(vec2(0.5, 0.5)), rand(vec2(0.1, 0.2)), rand(vec2(0.8, 0.6)), 1.0);
}