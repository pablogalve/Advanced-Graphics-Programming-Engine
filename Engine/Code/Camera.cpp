 #include "Camera.h"
#include "engine.h"
#include "GLFW/glfw3.h"

Camera::Camera()
{
}

Camera::Camera(glm::vec3 pos, glm::vec3 rot)
{
	position = pos;
	yaw = rot.x;
	pitch = rot.y;

	orbiting = false;
	target = glm::vec3(0, 0, 0);
	direction = glm::normalize(position - target);
	radius = 70.0f;
	rotationSpeed = 0.5f;
}

void Camera::HandleInput(App* app)
{
	if (!orbiting)
	{
		glm::vec3 posToMove = glm::vec3(0.0f, 0.0f, 0.0f);

		if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED)
		{
			this->yaw += app->input.mouseDelta.x * app->deltaTime * 12.f;
			this->pitch -= app->input.mouseDelta.y * app->deltaTime * 12.f;
		}

		// Move forward and backward
		if (app->input.keys[K_W] == BUTTON_PRESSED)
			posToMove += speed * forward;
		else if (app->input.keys[K_S] == BUTTON_PRESSED)
			posToMove += speed * -forward;

		// Move right and left
		if (app->input.keys[K_A] == BUTTON_PRESSED)
			posToMove += speed * right;
		else if (app->input.keys[K_D] == BUTTON_PRESSED)
			posToMove += speed * -right;

		// Move up and down
		if (app->input.keys[K_R] == BUTTON_PRESSED)
			posToMove += speed * up;
		else if (app->input.keys[K_F] == BUTTON_PRESSED)
			posToMove += speed * -up;

		position += posToMove;
	}

	RecalculateViewMatrix();
}

void Camera::RecalculateViewMatrix()
{
	if (orbiting)
	{
		float camX = sin(glfwGetTime() * rotationSpeed) * radius;
		float camZ = cos(glfwGetTime() * rotationSpeed) * radius;

		viewMatrix = glm::lookAt(
			glm::vec3(camX, 0.0f, camZ), 
			target, 
			glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else
	{
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		// Recalculate axis
		forward = glm::normalize(direction);
		right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
		up = glm::cross(forward, right);

		viewMatrix = glm::lookAt(
			position, 
			position + forward, 
			up);
	}	
}