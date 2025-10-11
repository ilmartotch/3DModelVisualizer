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

// Forward declarations
class ModelManager;
class SceneManager;
class SceneObject;

// Struttura per memorizzare i dati della mesh
struct MeshData {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    std::string materialName;
};

// Struttura per memorizzare i dati della texture
struct TextureData {
    unsigned int id=0;
    std::string type;
    std::string path;
};

class ImportedModel : public Model {
private:
    std::vector<MeshData> meshes;
    std::vector<TextureData> textures;
    std::string directory;
	std::string path;
    
    GLuint VAO = 0;
    std::vector<GLuint> vbos;
    GLuint EBO = 0;

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

public:
    ImportedModel(const std::string& name = "ImportedModel");
    virtual ~ImportedModel();
    
    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanup() override;
	virtual void setupVertexAttributes() override;
    
    // Aggiungi mesh al modello
    void addMesh(const MeshData& mesh);
    // Aggiungi texture
    void addTexture(const TextureData& texture);
    // Imposta la directory di base
    void setDirectory(const std::string& dir) { directory = dir; }
	// Imposta il path del modello
	void setPath(const std::string& modelPath) { path = modelPath; }
    
    // Getter per i dati del modello
    const std::vector<MeshData>& getMeshes() const { return meshes; }
    const std::vector<TextureData>& getTextures() const { return textures; }

    std::shared_ptr<Model> clone() const override;

	const std::string& getPath() const { return path; }
};

class ModelLoader {
private:
    static std::unordered_map<std::string, unsigned int> textureCache;

public:
    static bool openModelFile(ModelManager& modelManager, SceneManager& sceneManager, const glm::vec3& cameraPos, 
                        const glm::vec3& cameraTarget, bool& objectSelected, bool& showSelectedModelPanel);

    static bool openTextureFile(ModelManager& modelManager, SceneManager& sceneManager);
    
    // Carica un modello 3D da file utilizzando Assimp
    static std::shared_ptr<ImportedModel> loadModel(const std::string& path);
    
    // Carica una texture utilizzando stb_image
    static unsigned int loadTexture(const std::string& path);

private:
    // Processa un nodo del modello Assimp
    static void processNode(aiNode* node, const aiScene* scene, 
                          std::shared_ptr<ImportedModel> model, const std::string& directory);
    
    // Processa una mesh del modello Assimp
    static MeshData processMesh(aiMesh* mesh, const aiScene* scene);
    
    // Carica le texture associate al materiale
    static std::vector<TextureData> loadMaterialTextures(aiMaterial* mat, aiTextureType type, 
                                                const std::string& typeName, const std::string& directory,
                                                const aiScene* scene, const std::string& modelPath);

    // Normalizza i vertici del modello per adattarlo a una dimensione standard
    static void normalizeModel(std::shared_ptr<ImportedModel> model);
};