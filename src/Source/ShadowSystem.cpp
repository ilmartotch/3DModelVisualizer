#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include "../Include/ShadowSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <vector>
#include <iostream>
#include <limits>

ShadowSystem::ShadowSystem() {}

ShadowSystem::~ShadowSystem() {
    shutdown();
}

bool ShadowSystem::initialize(int shadowMapSize) {
    m_shadowMapSize = shadowMapSize;

    glGenFramebuffers(1, &m_depthMapFBO);

    glGenTextures(1, &m_depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowMapSize, m_shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer for shadow map is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void ShadowSystem::shutdown() {
    if (m_depthMapFBO != 0) {
        glDeleteFramebuffers(1, &m_depthMapFBO);
        m_depthMapFBO = 0;
    }
    if (m_depthMapTexture != 0) {
        glDeleteTextures(1, &m_depthMapTexture);
        m_depthMapTexture = 0;
    }
}

void ShadowSystem::setLightPosition(const glm::vec3& pos) {
    m_lightPos = pos;
}

ShadowSystem::SceneBounds ShadowSystem::computeSceneBounds(const SceneManager& scene, unsigned int skipObjectId) const {
    SceneBounds b;
    for (const auto& obj : scene.getObjects()) {
        if (!obj) continue;
        if (obj->getId() == skipObjectId) continue;

        const glm::vec3 p = obj->getPosition();
        const glm::vec3 s = obj->getScale();
        const glm::vec3 half = 0.5f * s;

        const glm::vec3 localMin = p - half;
        const glm::vec3 localMax = p + half;

        b.min = glm::min(b.min, localMin);
        b.max = glm::max(b.max, localMax);
        b.valid = true;
    }

    if (!b.valid) {
        b.min = glm::vec3(-2.0f);
        b.max = glm::vec3( 2.0f);
        b.valid = true;
    }

    if (b.min.y < m_floorHeight) b.min.y = m_floorHeight;

    return b;
}

void ShadowSystem::updateMatrices(const SceneManager& sceneManager, unsigned int lightObjectIdToExclude) {
    SceneBounds b = computeSceneBounds(sceneManager, lightObjectIdToExclude);
    const glm::vec3 centerWS = 0.5f * (b.min + b.max);

    const glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - m_lightPos);
    const float diag = glm::length(b.max - b.min);
    const float dist = (diag > 0.0001f ? diag : 10.0f);
    const glm::vec3 lightPosWS = centerWS - lightDir * dist;

    m_lightView = glm::lookAt(lightPosWS, centerWS, glm::vec3(0,1,0));

    std::vector<glm::vec3> corners = {
        {b.min.x, b.min.y, b.min.z}, {b.max.x, b.min.y, b.min.z},
        {b.min.x, b.max.y, b.min.z}, {b.max.x, b.max.y, b.min.z},
        {b.min.x, b.min.y, b.max.z}, {b.max.x, b.min.y, b.max.z},
        {b.min.x, b.max.y, b.max.z}, {b.max.x, b.max.y, b.max.z}
    };

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& c : corners) {
        glm::vec4 lc = m_lightView * glm::vec4(c, 1.0f);
        minX = std::min(minX, lc.x);
        maxX = std::max(maxX, lc.x);
        minY = std::min(minY, lc.y);
        maxY = std::max(maxY, lc.y);
        minZ = std::min(minZ, lc.z);
        maxZ = std::max(maxZ, lc.z);
    }

    const float pad = 1.0f;
    minX -= pad; maxX += pad;
    minY -= pad; maxY += pad;
    constexpr float zMult = 3.0f;
    if (minZ < 0) minZ *= zMult; else minZ /= zMult;
    if (maxZ < 0) maxZ /= zMult; else maxZ *= zMult;

    m_lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    m_lightSpaceMatrix = m_lightProj * m_lightView;
}

void ShadowSystem::renderDepthPass(SceneManager& sceneManager, GLuint depthShader, unsigned int lightObjectIdToExclude, int viewportWidth, int viewportHeight) {
    (void)viewportWidth;
    (void)viewportHeight;

    glUseProgram(depthShader);
    SetUniformMat4(depthShader, "lightSpaceMatrix", m_lightSpaceMatrix);

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    sceneManager.renderForDepth(depthShader, lightObjectIdToExclude);

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void ShadowSystem::bindForShading(GLuint shader, int textureUnit, bool useShadows) {
    if (useShadows) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_depthMapTexture);
        SetUniformInt(shader, "shadowMap", textureUnit);
        SetUniformMat4(shader, "lightSpaceMatrix", m_lightSpaceMatrix);
        SetUniformInt(shader, "useShadows", 1);
        

        SetUniformFloat(shader, "shadowBias", 0.005f);
    } else {
        SetUniformInt(shader, "useShadows", 0);
    }
}