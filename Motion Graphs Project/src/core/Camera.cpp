#include <core/Camera.h>

Camera::Camera(glm::vec3 _position, glm::vec3 _target, glm::vec3 _up, float _velocity) {
	position = _position;
	up = _up;
	velocity = _velocity;
	direction = glm::normalize(_target - _position);
}

glm::mat4 Camera::generateView() {
	return glm::lookAt(position, position + direction, up);
}

void Camera::processInput(GLFWwindow* window) {

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		position += velocity * flatten(&direction);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		position -= velocity * flatten(&direction);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		glm::vec3 d = glm::normalize(glm::cross(direction, up));
		position -= velocity * flatten(&d);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		glm::vec3 d = glm::normalize(glm::cross(direction, up));
		position += velocity * flatten(&d);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		position += velocity * up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		position -= velocity * up;
	}
}

void Camera::rotateYaw(float deg) {
	glm::vec4 direction_vec4(direction, 1.0f);
	direction_vec4 = glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(0.0, 1.0, 0.0)) * direction_vec4;
	direction = { direction_vec4.x, direction_vec4.y, direction_vec4.z };
}

void Camera::rotatePitch(float deg) {
	glm::vec4 direction_vec4(direction, 1.0f);
	const glm::vec3 axis = glm::cross(direction, up);
	direction_vec4 = glm::rotate(glm::mat4(1.0f), glm::radians(deg), axis) * direction_vec4;
	direction = { direction_vec4.x, direction_vec4.y, direction_vec4.z };
}

// Remove y component
glm::vec3 Camera::flatten(glm::vec3* vector) {
	glm::vec3 new_vector = { vector->x, 0.0f, vector->z };
	return new_vector;
}

void Camera::lookAt(glm::vec3& target) {
	direction = glm::normalize(target - position);
}

