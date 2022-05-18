#pragma once

#include "platform.h"

enum class EntityType
{
    PATRICK,
    PLANE,
    NONE
};

struct Entity
{
    Entity(glm::vec3 pos, glm::vec3 scaleFactor, u32 modelIndex, EntityType type);

    glm::mat4  worldMatrix;  // Coordinates of an object with respect to the world space
    u32        modelIndex;
    u32        localParamsOffset;
    u32        localParamsSize;
    EntityType type = EntityType::NONE;    
};

glm::mat4 TransformPositionScale(const glm::vec3& pos, const glm::vec3& scaleFactors);
glm::mat4 TransformRotation(const glm::mat4& matrix, float angle, glm::vec3 axis);