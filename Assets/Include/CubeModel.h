#pragma once
#include "../src/Include/Model.h"

class CubeModel : public Model {
public:
	CubeModel(const std::string& name = "Cube");
	virtual ~CubeModel();

	virtual void initialize() override;
	virtual void render() override;
	virtual void cleanup() override;
};