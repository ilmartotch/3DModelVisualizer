#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../Include/SceneManager.h"
#include "../Include/ModelManager.h"
#include "../Include/Model.h"

class ModelManager;
class SceneManager;
class SceneObject;
class ImportedModel;
class ImagePlaneModel;

struct MeshData {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    std::string materialName;
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct TextureData {
    unsigned int id = 0;
    std::string type;
    std::string path;
};

class ImportedModel : public Model {
private:

	friend class ModelLoader;

    std::vector<MeshData> meshes;
    std::vector<TextureData> textures;
    std::string directory;
    std::string path;

    struct MeshRenderData {
        GLuint VAO = 0;
        GLuint VBO_vertices = 0;
        GLuint VBO_normals = 0;
        GLuint VBO_texCoords = 0;
        GLuint EBO = 0;
        size_t indexCount = 0;
        GLuint textureID = 0;
        std::string materialName;
    };
    
    std::vector<MeshRenderData> meshRenderData;

    glm::vec3 normalizationCenter = glm::vec3(0.0f);
    float normalizationScale = 1.0f;

	void rebuildBaseGeometryData();
    
public:
    ImportedModel(const std::string& name = "ImportedModel");
    virtual ~ImportedModel();
    
    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanup() override;
    virtual void setupVertexAttributes() override;
    
    void addMesh(const MeshData& mesh);
    void addTexture(const TextureData& texture);
    void setDirectory(const std::string& dir) { directory = dir; }
    void setPath(const std::string& modelPath) { path = modelPath; }
    
    const std::vector<MeshData>& getMeshes() const { return meshes; }
    const std::vector<TextureData>& getTextures() const { return textures; }

    std::shared_ptr<Model> clone() const override;

    const std::string& getPath() const { return path; }

    void renderMesh(size_t meshIndex);
    
    size_t getMeshCount() const { return meshes.size(); }
    
    std::string getMeshName(size_t index) const;

    void setNormalizationCenter(const glm::vec3& center) { normalizationCenter = center; }
    void setNormalizationScale(float scale) { normalizationScale = scale; }
    glm::vec3 getNormalizationCenter() const { return normalizationCenter; }
    float getNormalizationScale() const { return normalizationScale; }

    GLuint getPrimaryTextureID() const {
        for (const auto& renderData : meshRenderData) {
            if (renderData.textureID != 0 && glIsTexture(renderData.textureID)) {
                return renderData.textureID;
            }
        }
        return getTextureID();
    }
};

class ModelLoader {
public:
    enum class LoadMode {
        SINGLE_OBJECT,
        SEPARATE_MESHES,
        BY_MATERIAL
    };
    
    static bool openModelFile(ModelManager& modelManager, SceneManager& sceneManager,
        const glm::vec3& cameraPos, const glm::vec3& cameraTarget,
        bool& objectSelected, bool& showSelectedModelPanel);

    static std::shared_ptr<ImportedModel> loadModel(const std::string& path);
    static unsigned int loadTexture(const std::string& path);

    static std::shared_ptr<ImagePlaneModel> loadImageAsPlane(const std::string& path, std::string& errorMessage);

    static bool openImageFile(ModelManager& modelManager, SceneManager& sceneManager, const glm::vec3& cameraPos, 
        const glm::vec3& cameraTarget, bool& objectSelected, 
        bool& showSelectedModelPanel, std::string& errorMessage);

    static bool openTextureFile(ModelManager& modelManager, SceneManager& sceneManager,
        std::string& outTexturePath, GLuint& outTextureID,
        bool& needsConfirmation);

    static bool applyTextureToSelected(SceneManager& sceneManager, GLuint textureID);

    using ProgressCallback = std::function<void(float, const std::string&)>;
    
    static bool openModelFileAdvanced(
        ModelManager& modelManager, 
        SceneManager& sceneManager,
        const glm::vec3& cameraPos, 
        const glm::vec3& cameraTarget,
        bool& objectSelected, 
        bool& showSelectedModelPanel,
        LoadMode mode = LoadMode::SINGLE_OBJECT,
        ProgressCallback progressCallback = nullptr);
    
private:
    static ProgressCallback s_progressCallback;
    
    static void reportProgress(float progress, const std::string& message);

    static void processNode(aiNode* node, const aiScene* scene, 
                           std::shared_ptr<ImportedModel> model, const std::string& directory,
                           const aiMatrix4x4& parentTransform);
    
    static MeshData processMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform);
    
    static std::vector<TextureData> loadMaterialTextures(aiMaterial* mat, aiTextureType type, 
        const std::string& typeName, const std::string& directory, 
        const aiScene* scene, const std::string& modelPath);
    
    static void normalizeModel(std::shared_ptr<ImportedModel> model);

    static void createSeparateMeshObjects(
        std::shared_ptr<ImportedModel> model,
        ModelManager& modelManager,
        SceneManager& sceneManager,
        const glm::vec3& basePosition,
        std::shared_ptr<SceneObject>& firstObject);

    static std::unordered_map<std::string, unsigned int> textureCache;

    static std::string openFileDialog(const wchar_t* filter);
    
    static bool replaceModelTexture(std::shared_ptr<Model> model, const std::string& texturePath);
};