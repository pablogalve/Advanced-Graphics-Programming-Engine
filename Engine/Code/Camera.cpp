#include "Camera.h"

Camera::Camera()
{
}

Camera::Camera(glm::vec3 pos, glm::vec3 rot)
{
	position = pos;
	yaw = rot.x;
	pitch = rot.y;
}

void Camera::Update(App* app)
{
	RecalculateViewMatrix();
}

void Camera::RecalculateViewMatrix()
{
	glm::vec3 orientation;
	orientation.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	orientation.y = sin(glm::radians(pitch));
	orientation.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	// Recalculate axis
	glm::vec3 forward = glm::normalize(orientation);
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
	glm::vec3 up = glm::cross(forward, right);

	viewMatrix = glm::lookAt(position, position + forward, up);
}
