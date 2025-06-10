#include "../src/Include/ModelManager.h"
#include <algorithm>

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