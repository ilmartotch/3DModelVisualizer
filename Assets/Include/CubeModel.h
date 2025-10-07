#pragma once
#include "../src/Include/Model.h"

class CubeModel : public Model {
public:
	CubeModel(const std::string& name = "Cube");
	virtual ~CubeModel();

	virtual void initialize() override;
	virtual void render() override;
	virtual void cleanup() override;
	
	std::shared_ptr<Model> clone() const override;

protected:
	virtual void setupVertexAttributes() override;
};