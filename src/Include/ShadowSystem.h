#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <limits>
#include "Shaders.h"
#include "SceneManager.h"

class ShadowSystem {
public:
    struct SceneBounds {
        glm::vec3 min{ std::numeric_limits<float>::max() };
        glm::vec3 max{ std::numeric_limits<float>::lowest() };
        bool valid = false;
    };

    ShadowSystem();
    ~ShadowSystem();

    bool initialize(int shadowMapSize);
    void shutdown();

    void setLightPosition(const glm::vec3& pos);
    void setFloorHeight(float h) { m_floorHeight = h; }
    
    void updateMatrices(const SceneManager& sceneManager, unsigned int lightObjectIdToExclude);

    void renderDepthPass(SceneManager& sceneManager, GLuint depthShader, unsigned int lightObjectIdToExclude, int viewportWidth, int viewportHeight);
    void bindForShading(GLuint shader, int textureUnit, bool useShadows);

    glm::mat4 getLightSpaceMatrix() const { return m_lightSpaceMatrix; }

    SceneBounds computeSceneBounds(const SceneManager& scene, unsigned int skipObjectId) const;

private:
    GLuint m_depthMapFBO = 0;
    GLuint m_depthMapTexture = 0;
    int m_shadowMapSize = 2048;

    glm::vec3 m_lightPos{ -2.0f, 4.0f, -1.0f };
    glm::mat4 m_lightProj;
    glm::mat4 m_lightView;
    glm::mat4 m_lightSpaceMatrix;
    float m_floorHeight = 0.0f;
};