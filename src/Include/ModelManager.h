#pragma once
#include "Model.h"
#include "../src/Include/ModelLoader.h" 
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <filesystem>

class ModelManager {
public:
	ModelManager();
	~ModelManager();

	void registerModel(std::shared_ptr<Model> model);

	void initializeModels();

	bool setActiveModel(const std::string& name);

	std::vector<std::string> getModelNames() const;

	void updateActiveModel(float currentTime);

	void renderActiveModel();

	void cleanup();

	bool hasActiveModel() const;

	void setActiveModelRenderMode(Model::RenderMode mode) {
		if (m_activeModel) {
			m_activeModel->setRenderMode(mode);
		}
	}

	std::shared_ptr<Model> getActiveModel() const {
		return m_activeModel;
	}

	std::shared_ptr<Model> getModel(const std::string& name) const {
		auto it = m_models.find(name);
		return (it != m_models.end()) ? it->second : nullptr;
	}

	// Metodo per caricare un modello da file
	std::shared_ptr<Model> loadModelFromFile(const std::string& filePath);

	// Metodo per caricare un'immagine 2D come texture
	unsigned int loadTextureFromFile(const std::string& filePath);

	// Controlla se un modello con il nome specificato esiste già
	bool modelExists(const std::string& name);

	// Genera un nome univoco per un modello
	std::string generateUniqueName(const std::string& baseName);

	// Ritorna i tipi di file supportati
	std::vector<std::string> getSupportedModelFormats();
	std::vector<std::string> getSupportedImageFormats();

private:
	std::map<std::string, std::shared_ptr<Model>> m_models;
	std::shared_ptr<Model> m_activeModel;

};