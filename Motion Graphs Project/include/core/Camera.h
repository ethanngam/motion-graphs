#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

class Camera
{

public:
	Camera(glm::vec3 _position, glm::vec3 _target, glm::vec3 _up, float velocity);
	glm::mat4 generateView();
	void processInput(GLFWwindow* window);
	void rotateYaw(float deg);
	void rotatePitch(float deg);
	void lookAt(glm::vec3& position);


private:
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 up;
	float velocity;

	glm::vec3 flatten(glm::vec3* vector);
};

