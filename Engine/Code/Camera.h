#pragma once

#include "platform.h"

struct App;

struct Camera {
    Camera();
    Camera(glm::vec3 pos, glm::vec3 rot);

    void Update(App* app);
    void RecalculateViewMatrix();

    float aspectRatio;
    float znear;
    float zfar;

    glm::vec3 position;
    glm::vec3 target;
    glm::mat4 projection; // Transform from view coordinates to clip coordinates
    glm::mat4 viewMatrix; // Transform from world coordinates to camera/eye/view coordinates

    // Rotation
    float yaw;
    float pitch;
};