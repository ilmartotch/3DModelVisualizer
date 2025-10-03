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
	// Se il nome è vuoto, imposta il modello attivo a nullptr
	if (name.empty()) {
		m_activeModel = nullptr;
		return true;
	}

	// Cerca il modello per nome
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
    // Verifica che il file esista
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Error: File does not exist: " << filePath << std::endl;
        return nullptr;
    }
    
    // Ottieni l'estensione del file
    std::string extension = std::filesystem::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Verifica se l'estensione è supportata
    std::vector<std::string> supportedFormats = getSupportedModelFormats();
    if (std::find(supportedFormats.begin(), supportedFormats.end(), extension) == supportedFormats.end()) {
        std::cerr << "Error: Unsupported file format: " << extension << std::endl;
        return nullptr;
    }
    
    // Ottieni il nome base del file
    std::string baseName = std::filesystem::path(filePath).stem().string();
    
    // Genera un nome univoco
    std::string uniqueName = generateUniqueName(baseName);
    
    // Carica il modello usando ModelLoader
    auto model = ModelLoader::loadModel(filePath);
    if (!model) {
        std::cerr << "Failed to load model: " << filePath << std::endl;
        return nullptr;
    }
    
    // Rinomina il modello con il nome univoco
    model->setName(uniqueName);
    
    // Inizializza il modello
    model->initialize();
    
    // Registra il modello
    registerModel(model);
    
    std::cout << "Model loaded successfully: " << uniqueName << std::endl;
    return model;
}

unsigned int ModelManager::loadTextureFromFile(const std::string& filePath) {
    // Verifica che il file esista
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Error: Texture file does not exist: " << filePath << std::endl;
        return 0;
    }
    
    // Ottieni l'estensione del file
    std::string extension = std::filesystem::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Verifica se l'estensione è supportata
    std::vector<std::string> supportedFormats = getSupportedImageFormats();
    if (std::find(supportedFormats.begin(), supportedFormats.end(), extension) == supportedFormats.end()) {
        std::cerr << "Error: Unsupported image format: " << extension << std::endl;
        return 0;
    }
    
    // Carica la texture usando ModelLoader
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
    
    // Continua ad aggiungere un numero finché non troviamo un nome univoco
    while (modelExists(uniqueName)) {
        uniqueName = baseName + "_" + std::to_string(counter++);
    }
    
    return uniqueName;
}

std::vector<std::string> ModelManager::getSupportedModelFormats() {
    // Elenco dei formati supportati da Assimp
    return {".obj", ".fbx", ".gltf", ".glb", ".dae", ".3ds", ".blend"};
}

std::vector<std::string> ModelManager::getSupportedImageFormats() {
    // Elenco dei formati supportati da stb_image
    return {".jpg", ".jpeg", ".png", ".bmp", ".tga"};
}