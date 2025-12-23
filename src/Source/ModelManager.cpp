#include "../src/Include/ModelManager.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

ModelManager::ModelManager() : m_activeModel(nullptr) {}

ModelManager::~ModelManager() {
	cleanup();
}

void ModelManager::registerModel(std::shared_ptr<Model> model) {
	if (model) {
		m_models[model->getName()] = model;
	}
}

void ModelManager::initializeModels() {
	for (auto& pair : m_models) {
		if (!pair.second->isInitialized()) {
			pair.second->initialize();
		}
	}
}

bool ModelManager::setActiveModel(const std::string& name) {
	if (name.empty()) {
		m_activeModel = nullptr;
		return true;
	}

	auto it = m_models.find(name);
	if (it != m_models.end()) {
		m_activeModel = it->second;
		return true;
	}

	return false;
}

std::vector<std::string> ModelManager::getModelNames() const {
	std::vector<std::string> names;
	names.reserve(m_models.size());

	for (const auto& pair : m_models) {
		names.push_back(pair.first);
	}

	return names;
}

void ModelManager::cleanup() {
	for (auto& pair : m_models) {
		if (pair.second) {
			pair.second->cleanup();
		}
	}
	m_models.clear();
	m_activeModel = nullptr;
}

bool ModelManager::hasActiveModel() const {
	return m_activeModel != nullptr;
}

void ModelManager::updateActiveModel(float currentTime) {
	if (hasActiveModel()) {
		m_activeModel->update(currentTime);
	}
}

void ModelManager::renderActiveModel() {
	if (hasActiveModel()) {
		m_activeModel->render();
	}
}

std::shared_ptr<Model> ModelManager::loadModelFromFile(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Error: File does not exist: " << filePath << std::endl;
        return nullptr;
    }

    std::string extension = std::filesystem::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    std::vector<std::string> supportedFormats = getSupportedModelFormats();
    if (std::find(supportedFormats.begin(), supportedFormats.end(), extension) == supportedFormats.end()) {
        std::cerr << "Error: Unsupported file format: " << extension << std::endl;
        return nullptr;
    }

    std::string baseName = std::filesystem::path(filePath).stem().string();
    std::string uniqueName = generateUniqueName(baseName);

    auto model = ModelLoader::loadModel(filePath);
    if (!model) {
        std::cerr << "Failed to load model: " << filePath << std::endl;
        return nullptr;
    }

    model->setName(uniqueName);
    model->initialize();
    registerModel(model);
    
    std::cout << "Model loaded successfully: " << uniqueName << std::endl;
    return model;
}

unsigned int ModelManager::loadTextureFromFile(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Error: Texture file does not exist: " << filePath << std::endl;
        return 0;
    }
    
    std::string extension = std::filesystem::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    std::vector<std::string> supportedFormats = getSupportedImageFormats();
    if (std::find(supportedFormats.begin(), supportedFormats.end(), extension) == supportedFormats.end()) {
        std::cerr << "Error: Unsupported image format: " << extension << std::endl;
        return 0;
    }
    
    unsigned int textureID = ModelLoader::loadTexture(filePath);
    if (textureID == 0) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
    }
    
    return textureID;
}

bool ModelManager::modelExists(const std::string& name) {
    for (const auto& pair : m_models) {
        if (pair.second && pair.second->getName() == name) {
            return true;
        }
    }
    return false;
}

std::string ModelManager::generateUniqueName(const std::string& baseName) {
    std::string uniqueName = baseName;
    int counter = 1;

    while (modelExists(uniqueName)) {
        uniqueName = baseName + "_" + std::to_string(counter++);
    }
    
    return uniqueName;
}

std::vector<std::string> ModelManager::getSupportedModelFormats() {
    return {".obj", ".fbx", ".gltf", ".glb", ".dae", ".3ds", ".blend"};
}

std::vector<std::string> ModelManager::getSupportedImageFormats() {
    return {".jpg", ".jpeg", ".png", ".bmp", ".tga"};
}

/*
ModelManager is a registry for all Model instances used in the scene.

Responsibilities:
- Register and initialize models including built-in primitives and imports
- Load models from disk via ModelLoader
- Generate unique names to avoid collisions when importing
- Provide model lookup by name for SceneManager

Models are stored as shared_ptr to allow multiple SceneObjects to reference
the same Model data, for example multiple cubes sharing one CubeModel.

Built-in models like Cube, Sphere and Pyramid are registered at startup.
Supported import formats are determined by Assimp library capabilities.
*/