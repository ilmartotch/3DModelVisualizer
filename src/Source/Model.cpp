#include "../Include/Model.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

GLuint Model::defaultTextureID = 0;

void Model::initializeDefaultTexture() {
    if (defaultTextureID == 0) {
        unsigned char pixelData[4] = {200, 200, 200, 255};
        
        glGenTextures(1, &defaultTextureID);
        glBindTexture(GL_TEXTURE_2D, defaultTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    }
}

Model::Model(const std::string& name) :
	m_vao(0),
	m_vbo(0),
	m_ebo(0),
	m_name(name),
	m_initialized(false),
	m_indexCount(0),
	textureID(0),
	useTexture(false),
	textureType(TextureType::NONE)
{
	initializeDefaultTexture();
}

Model::~Model() {
	cleanup();
}

void Model::cleanup() {
	if (m_vao != 0) {
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
	if (m_vbo != 0) {
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
	if (m_ebo !=0)
	{
		glDeleteBuffers(1, &m_ebo);
		m_ebo = 0;
	}
	m_initialized = false;
}

void Model::updateColors(float currentTime) {
	for (int i = 0; i < m_vertices.size() / 6; i++) {
		int colorOffset = i * 6 + 3;

		m_vertices[colorOffset] = 0.5f + 0.5f * sin(currentTime + i * 0.5f);
		m_vertices[colorOffset + 1] = 0.5f + 0.5f * sin(currentTime + i * 1.0f + 2.0f);
		m_vertices[colorOffset + 2] = 0.5f + 0.5f * sin(currentTime + i * 1.5f + 4.0f);
	}

	if (m_initialized) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(float), m_vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void Model::update(float currentTime) {
	updateColors(currentTime);
}

void Model::render() {
	if (m_vao == 0) return;
	
	glBindVertexArray(m_vao);
	
	glActiveTexture(GL_TEXTURE0);
	if (useTexture && textureID != 0) {
		glBindTexture(GL_TEXTURE_2D, textureID);
	} else {
		glBindTexture(GL_TEXTURE_2D, defaultTextureID);
	}
	
	if (m_indexCount > 0) {
		glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
	} else {
        GLsizei m_vertexCount = static_cast<GLsizei>(m_vertices.size() / 6);
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Model::setupBuffers() {
    if (m_vertices.empty()) return;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float),
        m_vertices.data(), GL_STATIC_DRAW);

    if (!m_indices.empty()) {
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int),
            m_indices.data(), GL_STATIC_DRAW);
        m_indexCount = static_cast<GLsizei>(m_indices.size());
    } else {
        m_indexCount = 0;
    }

    setupVertexAttributes();

    glBindVertexArray(0);
}

glm::vec2 Model::generatePlanarUV(const glm::vec3& position, UVMappingType plane) {
    switch (plane) {
        case UVMappingType::PLANAR_XY:
            return glm::vec2(position.x + 0.5f, position.y + 0.5f);
        case UVMappingType::PLANAR_XZ:
            return glm::vec2(position.x + 0.5f, position.z + 0.5f);
        case UVMappingType::PLANAR_YZ:
            return glm::vec2(position.y + 0.5f, position.z + 0.5f);
        default:
            return glm::vec2(0.0f, 0.0f);
    }
}

glm::vec2 Model::generateSphericalUV(const glm::vec3& position) {
    glm::vec3 normalized = glm::normalize(position);
    float u = 0.5f + (atan2f(normalized.z, normalized.x) / (2.0f * glm::pi<float>()));
    float v = 0.5f - (asinf(normalized.y) / glm::pi<float>());
    return glm::vec2(u, v);
}

glm::vec2 Model::generateCylindricalUV(const glm::vec3& position) {
    float u = 0.5f + (atan2f(position.z, position.x) / (2.0f * glm::pi<float>()));
    float v = position.y + 0.5f;
    return glm::vec2(u, v);
}

glm::vec2 Model::generateCubicUV(const glm::vec3& position, const glm::vec3& normal) {
    glm::vec3 absNormal = glm::abs(normal);
    
    if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z) {
        return glm::vec2(position.z + 0.5f, position.y + 0.5f);
    } else if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z) {
        return glm::vec2(position.x + 0.5f, position.z + 0.5f);
    } else {
        return glm::vec2(position.x + 0.5f, position.y + 0.5f);
    }
}

void Model::generateProceduralUVs(UVMappingType mappingType) {
    if (hasUVs) {
        return;
    }
    
    if (mappingType == UVMappingType::AUTO) {
        if (m_name.find("Sphere") != std::string::npos) {
            mappingType = UVMappingType::SPHERICAL;
        } else if (m_name.find("Cube") != std::string::npos) {
            mappingType = UVMappingType::CUBIC;
        } else if (m_name.find("Pyramid") != std::string::npos) {
            mappingType = UVMappingType::PLANAR_XZ;
        } else {
            mappingType = UVMappingType::PLANAR_XY;
        }
    }
    
    uvMappingType = mappingType;
    
    size_t floatsPerVertex = 6;
    size_t vertexCount = m_vertices.size() / floatsPerVertex;
    
    std::vector<float> newVertices;
    newVertices.reserve(vertexCount * 8);
    
    for (size_t i = 0; i < vertexCount; ++i) {
        size_t baseIndex = i * floatsPerVertex;
        
        for (size_t j = 0; j < 6; ++j) {
            newVertices.push_back(m_vertices[baseIndex + j]);
        }

        glm::vec3 position(m_vertices[baseIndex], m_vertices[baseIndex + 1], m_vertices[baseIndex + 2]);
        glm::vec3 normal(m_vertices[baseIndex + 3], m_vertices[baseIndex + 4], m_vertices[baseIndex + 5]);
        
        glm::vec2 uv;
        switch (mappingType) {
            case UVMappingType::SPHERICAL:
                uv = generateSphericalUV(position);
                break;
            case UVMappingType::CYLINDRICAL:
                uv = generateCylindricalUV(position);
                break;
            case UVMappingType::CUBIC:
                uv = generateCubicUV(position, normal);
                break;
            case UVMappingType::PLANAR_XY:
            case UVMappingType::PLANAR_XZ:
            case UVMappingType::PLANAR_YZ:
                uv = generatePlanarUV(position, mappingType);
                break;
            default:
                uv = glm::vec2(0.0f, 0.0f);
                break;
        }

        newVertices.push_back(uv.x);
        newVertices.push_back(uv.y);
    }
    
    m_vertices = newVertices;
    hasUVs = true;

    if (m_initialized) {
        glBindVertexArray(m_vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);

        setupVertexAttributes();
        
        glBindVertexArray(0);
    }
}