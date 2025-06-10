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

private:
	std::map<std::string, std::shared_ptr<Model>> m_models;
	std::shared_ptr<Model> m_activeModel;
};