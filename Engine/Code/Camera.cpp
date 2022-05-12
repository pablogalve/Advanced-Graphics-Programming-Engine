#include "Camera.h"
#include "engine.h"

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
	glm::vec3 posToMove = glm::vec3(0.0f, 0.0f, 0.0f);

	// Move forward and backward
	if (app->input.keys[K_W] == BUTTON_PRESSED)	
		posToMove = speed * forward;
	else if (app->input.keys[K_S] == BUTTON_PRESSED)
		posToMove = speed * -forward;

	// Move right and left
	if (app->input.keys[K_A] == BUTTON_PRESSED)
		posToMove = speed * right;
	else if (app->input.keys[K_D] == BUTTON_PRESSED)
		posToMove = speed * -right;

	// Move up and down
	if (app->input.keys[K_R] == BUTTON_PRESSED)
		posToMove = speed * up;
	else if (app->input.keys[K_F] == BUTTON_PRESSED)
		posToMove = speed * -up;

	position += posToMove;

	RecalculateViewMatrix();
}

void Camera::RecalculateViewMatrix()
{
	glm::vec3 orientation;
	orientation.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	orientation.y = sin(glm::radians(pitch));
	orientation.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	// Recalculate axis
	forward = glm::normalize(orientation);
	right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
	up = glm::cross(forward, right);

	viewMatrix = glm::lookAt(position, position + forward, up);
}
