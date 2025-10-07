#pragma once
#include "../src/Include/Model.h"

class PyramidModel : public Model {
public:
	PyramidModel(const std::string& name = "Pyramid");
	virtual ~PyramidModel();

	virtual void initialize() override;
	virtual void render() override;
	virtual void cleanup() override;

	std::shared_ptr<Model> clone() const override;

protected:
	virtual void setupVertexAttributes() override;
};