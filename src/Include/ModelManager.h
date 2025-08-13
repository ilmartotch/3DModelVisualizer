#pragma once
#include "Model.h"
#include <memory>
#include <vector>
#include <string>
#include <map>

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

private:
	std::map<std::string, std::shared_ptr<Model>> m_models;
	std::shared_ptr<Model> m_activeModel;
	std::shared_ptr<Model> getModel(const std::string& name) const {
		auto it = m_models.find(name);
		return (it != m_models.end()) ? it->second : nullptr;
	}

};