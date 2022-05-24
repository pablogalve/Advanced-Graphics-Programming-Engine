#pragma once

#include "platform.h"

struct App;

struct Camera {
    Camera();
    Camera(glm::vec3 pos, glm::vec3 rot);

    void HandleInput(App* app);
    void RecalculateViewMatrix();

    // Camera movement
    float speed = 1.0f;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;

    float aspectRatio;
    float znear;
    float zfar;

    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 direction;

    glm::mat4 projection; // Transform from view coordinates to clip coordinates
    glm::mat4 viewMatrix; // Transform from world coordinates to camera/eye/view coordinates

    // Rotation
    float yaw;
    float pitch;

    bool orbiting;
    float radius;
    float rotationSpeed;
};