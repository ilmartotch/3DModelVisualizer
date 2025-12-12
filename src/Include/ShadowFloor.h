#pragma once

#include "../Include/Model.h"

class ShadowFloor : public Model {
public:
    ShadowFloor(const std::string& name = "ShadowFloor");
    virtual ~ShadowFloor();

    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanup() override;
    virtual std::shared_ptr<Model> clone() const override;

protected:
    virtual void setupVertexAttributes() override;
};