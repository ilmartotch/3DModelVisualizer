#pragma once
#include "../src/Include/Model.h"

class SphereModel : public Model {
public:
	SphereModel(int sectors = 20, int stacks = 20, const std::string& name = "Sphere");
	virtual ~SphereModel();

	virtual void initialize() override;
	virtual void render() override;
	virtual void cleanup() override;

private:
	int m_sectors;
	int m_stacks;
};