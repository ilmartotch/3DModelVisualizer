#pragma once

#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Forward declarations
class SceneManager;
class ModelManager;
class SceneObject;
class Model;

namespace fs = std::filesystem;
class AssimpSceneExporter {
public: 
    struct ExportFormat {
        std::string id;
        std::string name;
        std::string extension;
        std::string description;
        std::string useCases;
        bool supportsMaterials;
        bool supportsTextures;
        bool supportsAnimations;
        bool isBinary;
        bool supportsEmbeddedTextures;
    };

    struct ExportResult {
        bool success = false;
        std::string errorMessage;
        std::string outputPath;
        unsigned int totalVertices = 0;
        unsigned int totalFaces = 0;
        unsigned int totalMeshes = 0;
        unsigned int totalMaterials = 0;
        unsigned int totalTextures = 0;
        size_t fileSize = 0;
        float exportTimeSeconds = 0.0f;
        bool texturesEmbedded = false;
        bool externalTexturesCopied = false;
        unsigned int texturesEmbeddedCount = 0;
        unsigned int texturesCopiedCount = 0;
        unsigned int texturesMissing = 0;
        std::vector<std::string> warnings;
    };

    struct TextureInfo {
        std::string objectName;
        std::string texturePath;
        std::string textureFilename;
        bool exists;
        size_t fileSize;
        bool willBeCopied;

        TextureInfo() : exists(false), fileSize(0), willBeCopied(false) {}
    };

    struct ExportPreview {
        std::vector<TextureInfo> textures;
        unsigned int totalObjects = 0;
        unsigned int totalVertices = 0;
        unsigned int totalFaces = 0;
        size_t estimatedTotalSize = 0;
        int texturesFound = 0;
        int texturesMissing = 0;
        int texturesUnique = 0;
    };

    static std::vector<ExportFormat> getSupportedFormats();

    static ExportPreview generateExportPreview(
        const SceneManager& sceneManager,
        const ModelManager& modelManager,
        bool copyTextures = true
    );

    static ExportResult exportScene(
        const SceneManager& sceneManager,
        const ModelManager& modelManager,
        const std::string& outputPath,
        const std::string& formatId,
        bool copyTextures = true,
        bool embedTextures = true
    );

private:
    static std::unordered_map<unsigned int, std::string> buildTexturePathMap(
        const SceneManager& sceneManager
    );

    static aiScene* buildAssimpScene(
        const SceneManager& sceneManager,
        ExportResult& result
    );
    
    static aiMesh* convertModelToMesh(
        const Model* model,
        const std::string& meshName,
        unsigned int& vertexOffset
    );
    
    static aiMaterial* createMaterialFromObject(
        const SceneObject* obj,
        const std::string& materialName,
        const std::string& textureRelativePath = ""
    );

    static aiMatrix4x4 glmToAiMatrix(const glm::mat4& mat);

    static bool copyTexturesWithTracking(
        const std::unordered_map<unsigned int, std::string>& textureMap,
        const fs::path& exportDir,
        std::string& error
    );

    static void embedTexturesIntoScene(
        aiScene* scene,
        const std::unordered_map<unsigned int, std::string>& textureMap,
        ExportResult& result
    );

    static void calculateSceneStats(const aiScene* scene, ExportResult& result);
};