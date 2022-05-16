#include "entity.h"

Entity::Entity(glm::vec3 pos, glm::vec3 scaleFactor, u32 modelIndex, EntityType type)
{
    this->worldMatrix = TransformPositionScale(pos, scaleFactor);
    this->modelIndex = modelIndex;
    this->type = type;
}

glm::mat4 TransformPositionScale(const glm::vec3& pos, const glm::vec3& scaleFactors)
{
    glm::mat4 transform = glm::translate(pos);
    transform = glm::scale(transform, scaleFactors);
    return transform;
}

glm::mat4 TransformRotation(const glm::mat4& matrix, float angle, glm::vec3 axis)
{
    return glm::mat4();
}
