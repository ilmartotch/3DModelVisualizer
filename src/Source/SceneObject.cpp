#include "../Include/SceneObject.h"

void SceneObject::setOverrideTexture(GLuint texID) {
    overrideTextureID = texID;
    hasOverrideTexture_ = true;
    clearOverrideColor();
}

void SceneObject::setOverrideColor(const glm::vec4& color) {
    overrideColor = color;
    hasOverrideColor_ = true;
    clearOverrideTexture();
}

void SceneObject::clearOverrideTexture() {
    if (hasOverrideTexture_ && overrideTextureID != 0) {}
    overrideTextureID = 0;
    hasOverrideTexture_ = false;
}

void SceneObject::clearOverrideColor() {
    overrideColor = glm::vec4(1.0f);
    hasOverrideColor_ = false;
}