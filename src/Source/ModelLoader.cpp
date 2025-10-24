#include "../Include/ModelLoader.h"
#include "../Include/TextureManager.h"
#include "../../Assets/Include/ImagePlaneModel.h"
#include <iostream>
#include <filesystem>
#include <stb_image.h>
#include <limits>
#include <algorithm>
#include <thread>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>
#endif

// Callback globale per progresso
ModelLoader::ProgressCallback ModelLoader::s_progressCallback = nullptr;

void ModelLoader::reportProgress(float progress, const std::string& message) {
    if (s_progressCallback) {
        s_progressCallback(progress, message);
    }
}

bool ModelLoader::openModelFile(ModelManager& modelManager, SceneManager& sceneManager,
    const glm::vec3& cameraPos, const glm::vec3& cameraTarget,
    bool& objectSelected, bool& showSelectedModelPanel) {

#ifdef _WIN32
    OPENFILENAME ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(glfwGetCurrentContext());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Models\0*.obj;*.fbx;*.dae;*.3ds;*.gltf;*.glb\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        std::string filePath = ofn.lpstrFile;

        try {
            // Usa il nome del file come nome del modello
            std::string modelName = std::filesystem::path(filePath).stem().string();

            // Carica il modello
            auto loadedModel = loadModel(filePath);
            if (loadedModel) {
                // Registra il modello
                modelManager.registerModel(loadedModel);
                loadedModel->initialize();

                // Calcola una posizione di spawn valida usando la logica centralizzata
                glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
                    cameraPos, cameraTarget, 1.0f, 1.0f);

                // Crea un'istanza del modello nella scena
                auto newObj = sceneManager.addObject(modelName, spawnPos);

                if (newObj) {
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                    return true;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Errore nel caricamento del modello: " << e.what() << std::endl;
        }
    }
    return false;
#else
    (void)modelManager; (void)sceneManager; (void)cameraPos; (void)cameraTarget; (void)objectSelected; (void)showSelectedModelPanel;
    std::cerr << "openModelFile non implementato per questa piattaforma." << std::endl;
    return false;
#endif
}

// Gestione coerente del caricamento texture
bool ModelLoader::openTextureFile(ModelManager& modelManager, SceneManager& sceneManager,
                                 std::string& outTexturePath, GLuint& outTextureID,
                                 bool& needsConfirmation) {
#ifdef _WIN32
    OPENFILENAME ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(glfwGetCurrentContext());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        std::string filePath = ofn.lpstrFile;
        outTexturePath = filePath;

        // Verifica che ci sia un oggetto selezionato
        auto selectedObj = sceneManager.getSelectedObject();
        if (!selectedObj) {
            std::cerr << "ERRORE: Seleziona un oggetto prima di applicare una texture." << std::endl;
            return false;
        }

        std::cout << "\n=== CARICAMENTO TEXTURE ===" << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Oggetto: " << selectedObj->getName() << std::endl;
        
        bool hasExistingOverride = selectedObj->hasOverrideTexture();
        std::shared_ptr<Model> model = selectedObj->getModel();
        bool hasModelTexture = (model && model->hasTexture() && model->getTextureID() != 0);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        while (glGetError() != GL_NO_ERROR);

        GLuint textureID = loadTexture(filePath);
        if (textureID == 0) {
            std::cerr << "ERRORE: Impossibile caricare la texture: " << filePath << std::endl;
            return false;
        }
        
        outTextureID = textureID;
        std::cout << "Texture caricata con successo (ID: " << textureID << ")" << std::endl;

        if (hasExistingOverride || hasModelTexture) {
            needsConfirmation = true;
            return true; // chiederà conferma nel layer UI
        }

        if (model && !model->hasUVCoordinates()) {
            std::cout << "Il modello non ha coordinate UV, generazione automatica..." << std::endl;
            model->generateProceduralUVs(Model::UVMappingType::AUTO);
        }
        
        selectedObj->setOverrideTexture(textureID);
        selectedObj->clearOverrideColor();
        
        std::cout << "Texture applicata con successo!" << std::endl;
        std::cout << "   - ID texture: " << textureID << std::endl;
        std::cout << "   - Oggetto: " << selectedObj->getName() << std::endl;
        std::cout << "==========================\n" << std::endl;
        
        needsConfirmation = false;
        return true;
    }
    
    needsConfirmation = false;
    return false;
#else
    (void)modelManager; (void)sceneManager; (void)outTexturePath; (void)outTextureID; (void)needsConfirmation;
    std::cerr << "openTextureFile non implementato per questa piattaforma." << std::endl;
    return false;
#endif
}

bool ModelLoader::applyTextureToSelected(SceneManager& sceneManager, GLuint textureID) {
    auto selectedObj = sceneManager.getSelectedObject();
    if (!selectedObj) {
        std::cerr << "ERRORE: Nessun oggetto selezionato." << std::endl;
        return false;
    }
    
    if (textureID == 0) {
        std::cerr << "ERRORE: ID texture non valido (0)." << std::endl;
        return false;
    }

    std::shared_ptr<Model> model = selectedObj->getModel();
    if (model && !model->hasUVCoordinates()) {
        std::cout << "Il modello non ha coordinate UV, generazione automatica..." << std::endl;
        model->generateProceduralUVs(Model::UVMappingType::AUTO);
    }
    
    selectedObj->clearOverrideColor();
    selectedObj->setOverrideTexture(textureID);
    
    std::cout << "Texture applicata con successo!" << std::endl;
    std::cout << "   - ID texture: " << textureID << std::endl;
    std::cout << "   - Oggetto: " << selectedObj->getName() << std::endl;
    std::cout << "==========================\n" << std::endl;
    
    return true;
}

// Inizializzazione del cache delle texture
std::unordered_map<std::string, unsigned int> ModelLoader::textureCache;

ImportedModel::ImportedModel(const std::string& name) : Model(name) {
}

ImportedModel::~ImportedModel() {
    cleanup();
}

void ImportedModel::initialize() {
    if (meshes.empty()) {
        std::cerr << "ERROR: No mesh data for model: " << getName() << std::endl;
        return;
    }
    
    meshRenderData.clear();
    meshRenderData.reserve(meshes.size());
    
    std::cout << "=== Inizializzazione " << getName() << " ===" << std::endl;
    std::cout << "  Mesh: " << meshes.size() << std::endl;
    std::cout << "  Texture disponibili: " << textures.size() << std::endl;
    
    for (size_t i = 0; i < textures.size(); ++i) {
        std::cout << "    Texture[" << i << "]: ID=" << textures[i].id 
                  << " tipo=" << textures[i].type 
                  << " path=" << textures[i].path << std::endl;
    }
    
    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];
        MeshRenderData renderData;
        
        // 1. VAO
        glGenVertexArrays(1, &renderData.VAO);
        glBindVertexArray(renderData.VAO);
        
        // 2. VBO vertici
        glGenBuffers(1, &renderData.VBO_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, renderData.VBO_vertices);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), 
                     mesh.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
        
        // 3. VBO normali
        if (!mesh.normals.empty()) {
            glGenBuffers(1, &renderData.VBO_normals);
            glBindBuffer(GL_ARRAY_BUFFER, renderData.VBO_normals);
            glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(float), 
                         mesh.normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);
        }
        
        // 4. VBO texture coordinates
        if (!mesh.texCoords.empty()) {
            glGenBuffers(1, &renderData.VBO_texCoords);
            glBindBuffer(GL_ARRAY_BUFFER, renderData.VBO_texCoords);
            glBufferData(GL_ARRAY_BUFFER, mesh.texCoords.size() * sizeof(float), 
                         mesh.texCoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);
            
            std::cout << "  Mesh " << i << " ha " << mesh.texCoords.size() / 2 << " coordinate UV" << std::endl;
        } else {
            std::cout << "  ATTENZIONE: Mesh " << i << " non ha coordinate UV!" << std::endl;
        }
        
        // 5. EBO indici
        if (!mesh.indices.empty()) {
            glGenBuffers(1, &renderData.EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderData.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                         mesh.indices.size() * sizeof(unsigned int),
                         mesh.indices.data(), GL_STATIC_DRAW);
            renderData.indexCount = mesh.indices.size();
        }
        
        // 6. Assegna texture alla mesh
        renderData.textureID = 0;
        renderData.materialName = mesh.materialName;
        
        if (i < textures.size() && textures[i].type == "texture_diffuse") {
            renderData.textureID = textures[i].id;
            std::cout << "  Mesh " << i << " <- Texture diretta ID " << renderData.textureID << std::endl;
        } else if (!textures.empty()) {
            for (const auto& tex : textures) {
                if (tex.type == "texture_diffuse") {
                    renderData.textureID = tex.id;
                    std::cout << "  Mesh " << i << " <- Texture condivisa ID " << renderData.textureID << std::endl;
                    break;
                }
            }
        }
        
        if (renderData.textureID != 0 && glIsTexture(renderData.textureID)) {
            std::cout << "  Mesh " << i << " texture validata (ID: " << renderData.textureID << ")" << std::endl;
        } else if (renderData.textureID != 0) {
            std::cerr << "  ERRORE: Mesh " << i << " texture ID " << renderData.textureID << " NON VALIDA!" << std::endl;
            renderData.textureID = 0;
        } else {
            std::cout << "  Mesh " << i << " senza texture" << std::endl;
        }
        
        meshRenderData.push_back(renderData);
        
        std::cout << "  Mesh " << i << " initialized: " 
                  << mesh.vertices.size() / 3 << " verts, "
                  << renderData.indexCount << " indices, "
                  << "textureID=" << renderData.textureID << std::endl;
    }
    
    glBindVertexArray(0);
    
    if (!textures.empty() && textures[0].id != 0) {
        setTexture(textures[0].id);
        std::cout << "  Texture principale modello impostata: " << textures[0].id << std::endl;
    }
    
    m_initialized = true;
    std::cout << "=== Fine inizializzazione " << getName() << " ===" << std::endl;
}

void ImportedModel::renderMesh(size_t meshIndex) {
    if (meshIndex >= meshRenderData.size()) {
        std::cerr << "Invalid mesh index: " << meshIndex << std::endl;
        return;
    }
    
    const auto& renderData = meshRenderData[meshIndex];
    glBindVertexArray(renderData.VAO);
    
    if (renderData.indexCount > 0) {
        glDrawElements(GL_TRIANGLES, renderData.indexCount, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, meshes[meshIndex].vertices.size() / 3);
    }
    
    glBindVertexArray(0);
}

void ImportedModel::render() {
    if (!m_initialized || meshRenderData.empty()) {
        std::cerr << "Model not initialized: " << getName() << std::endl;
        return;
    }
    
    for (size_t i = 0; i < meshRenderData.size(); ++i) {
        const auto& renderData = meshRenderData[i];
        
        glBindVertexArray(renderData.VAO);
        
        if (renderData.textureID != 0 && glIsTexture(renderData.textureID)) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderData.textureID);
        }
        
        if (renderData.indexCount > 0) {
            glDrawElements(GL_TRIANGLES, renderData.indexCount, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, meshes[i].vertices.size() / 3);
        }
    }
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::string ImportedModel::getMeshName(size_t index) const {
    if (index >= meshes.size()) return "Invalid";
    if (!meshes[index].materialName.empty()) {
        return meshes[index].materialName;
    }
    return getName() + "_Mesh" + std::to_string(index);
}

void ImportedModel::cleanup() {
    for (auto& renderData : meshRenderData) {
        if (renderData.VAO != 0) glDeleteVertexArrays(1, &renderData.VAO);
        if (renderData.VBO_vertices != 0) glDeleteBuffers(1, &renderData.VBO_vertices);
        if (renderData.VBO_normals != 0) glDeleteBuffers(1, &renderData.VBO_normals);
        if (renderData.VBO_texCoords != 0) glDeleteBuffers(1, &renderData.VBO_texCoords);
        if (renderData.EBO != 0) glDeleteBuffers(1, &renderData.EBO);
    }
    meshRenderData.clear();
    for (auto& texture : textures) {
        glDeleteTextures(1, &texture.id);
    }
    m_initialized = false;
}

void ImportedModel::addMesh(const MeshData& mesh) {
    meshes.push_back(mesh);
}

void ImportedModel::addTexture(const TextureData& texture) {
    textures.push_back(texture);
}

std::shared_ptr<Model> ImportedModel::clone() const {
    auto newModel = std::make_shared<ImportedModel>(getName());
    for (const auto& mesh : meshes) newModel->addMesh(mesh);
    for (const auto& texture : textures) newModel->addTexture(texture);
    newModel->setDirectory(directory);
    newModel->setPath(path);
    if (!textures.empty()) newModel->setTexture(textures[0].id);
    return newModel;
}

// Applica trasformazioni di nodo ricorsivamente
void ModelLoader::processNode(aiNode* node, const aiScene* scene, 
                            std::shared_ptr<ImportedModel> model, const std::string& directory,
                            const aiMatrix4x4& parentTransform) {
    std::cout << "Processando nodo: " << node->mName.C_Str() << " con " 
              << node->mNumMeshes << " mesh e " << node->mNumChildren << " figli" << std::endl;

    aiMatrix4x4 currentTransform = parentTransform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::cout << "Processando mesh " << i << ": " << mesh->mName.C_Str() 
                  << " con indice materiale " << mesh->mMaterialIndex << std::endl;
        
        model->addMesh(processMesh(mesh, scene, currentTransform));
        
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::cout << "Materiale trovato: " << material->GetName().C_Str() << std::endl;
            
            auto diffuseMaps = loadMaterialTextures(
                material, aiTextureType_DIFFUSE, "texture_diffuse", directory, scene, model->getPath());
            std::cout << "Caricate " << diffuseMaps.size() << " texture diffuse" << std::endl;
            for (const auto& texture : diffuseMaps) {
                model->addTexture(texture);
                if (texture.type == "texture_diffuse" && !model->hasTexture()) {
                    model->setTexture(texture.id);
                    std::cout << "Texture principale impostata: ID=" << texture.id << std::endl;
                }
            }
            
            auto specularMaps = loadMaterialTextures(
                material, aiTextureType_SPECULAR, "texture_specular", directory, scene, model->getPath());
            std::cout << "Caricate " << specularMaps.size() << " texture speculari" << std::endl;
            for (const auto& texture : specularMaps) {
                model->addTexture(texture);
            }
            
            if (diffuseMaps.empty() && specularMaps.empty()) {
                aiColor3D color(0.f, 0.f, 0.f);
                if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                    std::cout << "Nessuna texture trovata, uso colore del materiale: (" 
                              << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                    TextureData colorTexture;
                    colorTexture.type = "texture_diffuse";
                    colorTexture.path = "generated_color";
                    colorTexture.id = TextureManager::getInstance().createColorTexture(
                        glm::vec4(color.r, color.g, color.b, 1.0f));
                    model->addTexture(colorTexture);
                    model->setTexture(colorTexture.id);
                }
            }
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model, directory, currentTransform);
    }
}

MeshData ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform) {
    MeshData meshData;
    
    std::cout << "Elaborando mesh con " << mesh->mNumVertices << " vertici" << std::endl;

    aiMatrix3x3 normalMatrix = aiMatrix3x3(transform);
    normalMatrix = normalMatrix.Inverse().Transpose();

    meshData.vertices.reserve(mesh->mNumVertices * 3);
    meshData.normals.reserve(mesh->mNumVertices * 3);
    meshData.texCoords.reserve(mesh->mNumVertices * 2);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D p = mesh->mVertices[i];
        p = transform * p;

        meshData.vertices.push_back(p.x);
        meshData.vertices.push_back(p.y);
        meshData.vertices.push_back(p.z);

        if (mesh->HasNormals()) {
            aiVector3D n = mesh->mNormals[i];
            n = normalMatrix * n;
            n.Normalize();

            meshData.normals.push_back(n.x);
            meshData.normals.push_back(n.y);
            meshData.normals.push_back(n.z);
        }

        if (mesh->mTextureCoords[0]) {
            meshData.texCoords.push_back(mesh->mTextureCoords[0][i].x);
            meshData.texCoords.push_back(mesh->mTextureCoords[0][i].y);

            if (i < 5 || i > mesh->mNumVertices - 5) {
                std::cout << "  TexCoord[" << i << "]: (" 
                          << mesh->mTextureCoords[0][i].x << ", " 
                          << mesh->mTextureCoords[0][i].y << ")" << std::endl;
            }
        } else {
            meshData.texCoords.push_back(0.0f);
            meshData.texCoords.push_back(0.0f);
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            meshData.indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {
        meshData.materialName = "material_" + std::to_string(mesh->mMaterialIndex);
    }

    std::cout << "Mesh elaborata con " << meshData.vertices.size() / 3 << " vertici, " 
              << meshData.texCoords.size() / 2 << " coordinate UV, e " 
              << meshData.indices.size() << " indici" << std::endl;

    return meshData;
}

std::shared_ptr<ImportedModel> ModelLoader::loadModel(const std::string& path) {
    std::cout << "Caricamento modello: " << path << std::endl;
    
    auto model = std::make_shared<ImportedModel>(std::filesystem::path(path).stem().string());
    
    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate | 
                         aiProcess_GenSmoothNormals |
                         aiProcess_CalcTangentSpace |
                         aiProcess_JoinIdenticalVertices |
                         aiProcess_SortByPType;
    
    const aiScene* scene = importer.ReadFile(path, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }
    
    std::string directory = std::filesystem::path(path).parent_path().string();
    
    std::cout << "Directory del modello: " << directory << std::endl;
    model->setDirectory(directory);
    model->setPath(path);
    
    processNode(scene->mRootNode, scene, model, directory, aiMatrix4x4());
    normalizeModel(model);
    
    bool hasValidUVs = false;
    for (const auto& mesh : model->getMeshes()) {
        if (!mesh.texCoords.empty()) { hasValidUVs = true; break; }
    }
    if (!hasValidUVs) {
        std::cout << "ATTENZIONE: Il modello non ha coordinate UV, genera UV procedurali per permettere texturing" << std::endl;
        model->generateProceduralUVs(Model::UVMappingType::AUTO);
    }
    
    if (model->getTextures().empty()) {
        std::cout << "Nessuna texture trovata per il modello, assegno texture di default" << std::endl;
        TextureData defaultTex;
        defaultTex.id = TextureManager::getInstance().getDefaultTexture();
        defaultTex.type = "texture_diffuse";
        defaultTex.path = "default";
        model->addTexture(defaultTex);
        model->setTexture(defaultTex.id);
    }

    std::cout << "Modello caricato con successo: " << model->getName()
        << " con " << model->getMeshes().size() << " mesh e "
        << model->getTextures().size() << " texture" << std::endl;

    return model;
}

void ModelLoader::createSeparateMeshObjects(
    std::shared_ptr<ImportedModel> model,
    ModelManager& modelManager,
    SceneManager& sceneManager,
    const glm::vec3& basePosition,
    std::shared_ptr<SceneObject>& firstObject) {
    
    size_t meshCount = model->getMeshCount();
    std::string baseName = model->getName();
    
    std::cout << "=== Creazione " << meshCount << " oggetti separati ===" << std::endl;
    
    const auto& meshes = model->getMeshes();
    const auto& textures = model->getTextures();
    
    // Estrai parametri di normalizzazione dal modello originale (già applicati ai vertici)
    glm::vec3 normCenter = model->getNormalizationCenter();
    float normScale = model->getNormalizationScale();
    
    std::cout << "  Parametri normalizzazione ereditati:" << std::endl;
    std::cout << "    Centro: (" << normCenter.x << ", " << normCenter.y << ", " << normCenter.z << ")" << std::endl;
    std::cout << "    Scala: " << normScale << std::endl;
    
    for (size_t i = 0; i < meshCount; ++i) {
        std::string meshName = model->getMeshName(i);
        std::string uniqueModelName = baseName + "_mesh_" + std::to_string(i);

        // Calcola AABB e centro della MESH i (nei vertici già normalizzati)
        glm::vec3 minV( std::numeric_limits<float>::max());
        glm::vec3 maxV(-std::numeric_limits<float>::max());
        const auto& srcMesh = meshes[i];

        for (size_t vi = 0; vi < srcMesh.vertices.size(); vi += 3) {
            minV.x = std::min(minV.x, srcMesh.vertices[vi + 0]);
            minV.y = std::min(minV.y, srcMesh.vertices[vi + 1]);
            minV.z = std::min(minV.z, srcMesh.vertices[vi + 2]);

            maxV.x = std::max(maxV.x, srcMesh.vertices[vi + 0]);
            maxV.y = std::max(maxV.y, srcMesh.vertices[vi + 1]);
            maxV.z = std::max(maxV.z, srcMesh.vertices[vi + 2]);
        }
        glm::vec3 meshCenter = (minV + maxV) * 0.5f;

        // Ricentra i vertici della mesh intorno al suo centro per creare un modello locale coerente
        MeshData recenteredMesh = srcMesh;
        for (size_t vi = 0; vi < recenteredMesh.vertices.size(); vi += 3) {
            recenteredMesh.vertices[vi + 0] -= meshCenter.x;
            recenteredMesh.vertices[vi + 1] -= meshCenter.y;
            recenteredMesh.vertices[vi + 2] -= meshCenter.z;
        }

        // Crea un nuovo modello contenente SOLO questa mesh ricentrata
        auto singleMeshModel = std::make_shared<ImportedModel>(uniqueModelName);
        singleMeshModel->addMesh(recenteredMesh);

        // Eredita i parametri di normalizzazione
        singleMeshModel->setNormalizationCenter(normCenter);
        singleMeshModel->setNormalizationScale(normScale);
        
        // Texture: scegli una diffuse disponibile
        GLuint meshTextureID = 0;
        bool textureFound = false;
        std::string materialName = srcMesh.materialName;

        for (const auto& tex : textures) {
            if (tex.type == "texture_diffuse" && glIsTexture(tex.id)) {
                meshTextureID = tex.id;
                singleMeshModel->addTexture(tex);
                textureFound = true;
                std::cout << "    Texture diffuse ID " << tex.id 
                          << " associata (materiale: " << materialName << ")" << std::endl;
                break;
            }
        }
        if (!textureFound && !textures.empty()) {
            for (const auto& tex : textures) {
                if (glIsTexture(tex.id)) {
                    meshTextureID = tex.id;
                    singleMeshModel->addTexture(tex);
                    textureFound = true;
                    std::cout << "    Usando texture generica ID " << tex.id << std::endl;
                    break;
                }
            }
        }
        singleMeshModel->setTexture(meshTextureID);
        if (meshTextureID != 0) {
            std::cout << "    Texture principale impostata: " << meshTextureID << std::endl;
        } else {
            std::cout << "    ATTENZIONE: Nessuna texture disponibile per mesh " << i << std::endl;
        }

        singleMeshModel->setDirectory(model->getPath());

        // Registra e inizializza
        modelManager.registerModel(singleMeshModel);
        singleMeshModel->initialize();
        if (!singleMeshModel->isInitialized()) {
            std::cerr << "ERRORE: Inizializzazione mesh " << i << " fallita!" << std::endl;
            continue;
        }

        // POSIZIONAMENTO RELATIVO CORRETTO:
        // Posiziona il SceneObject nel centro originario della mesh (normalizzato),
        // mentre i vertici sono ricentrati localmente -> world = base + center + (v - center)
        glm::vec3 spawnPos = basePosition + meshCenter;

        // Crea SceneObject
        std::string objName = baseName + "_" + meshName;
        auto newObj = sceneManager.addObject(uniqueModelName, spawnPos);
        
        if (newObj) {
            if (i == 0) {
                firstObject = newObj;
            }

            auto objModel = newObj->getModel();
            bool finalTextureCheck = objModel->hasTexture() && objModel->getTextureID() != 0;

            std::cout << "  Creato: " << objName << " @ (" 
                      << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")"
                      << " [Texture: " << (finalTextureCheck ? "OK" : "MISSING") << "]";
            if (finalTextureCheck) {
                std::cout << " (ID: " << objModel->getTextureID() << ")";
            }
            std::cout << std::endl;
        } else {
            std::cerr << "  ERRORE: Creazione SceneObject fallita per mesh " << i << std::endl;
        }
    }
    
    std::cout << "=== Completata creazione oggetti separati ===" << std::endl;
}
unsigned int ModelLoader::loadTexture(const std::string& path) {
    // Cache
    if (textureCache.find(path) != textureCache.end()) {
        std::cout << "Texture trovata nella cache: " << path << std::endl;
        return textureCache[path];
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "Errore OpenGL precedente ripulito: 0x" << std::hex << error << std::dec << std::endl;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    if (textureID == 0) {
        std::cerr << "ERRORE: glGenTextures ha fallito" << std::endl;
        return 0;
    }

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        std::cerr << "stbi_failure_reason: " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }

    GLenum internalFormat, format;
    if (nrComponents == 1) { internalFormat = GL_R8;    format = GL_RED; }
    else if (nrComponents == 3) { internalFormat = GL_RGB8;  format = GL_RGB; }
    else if (nrComponents == 4) { internalFormat = GL_RGBA8; format = GL_RGBA; }
    else {
        std::cerr << "Formato immagine non supportato: " << nrComponents << " componenti" << std::endl;
        stbi_image_free(data);
        glDeleteTextures(1, &textureID);
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Errore OpenGL prima di glTexImage2D: 0x" << std::hex << error << std::dec << std::endl;
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureID);
        return 0;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Errore OpenGL dopo glTexImage2D: 0x" << std::hex << error << std::dec << std::endl;
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureID);
        return 0;
    }

    glFlush();
    glGenerateMipmap(GL_TEXTURE_2D);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Errore durante glGenerateMipmap: 0x" << std::hex << error << std::dec << std::endl;
        std::cerr << "Continuando senza mipmaps..." << std::endl;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glGetError();
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Errore OpenGL finale: 0x" << std::hex << error << std::dec << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }

    textureCache[path] = textureID;
    std::cout << "Texture caricata e cached con successo (ID: " << textureID << ")" << std::endl;
    return textureID;
}

// Helper per generare hash del contenuto texture (per embedded)
static std::string generateTextureHash(const unsigned char* data, size_t size) {
    std::stringstream ss;
    ss << std::hex;
    size_t sampleSize = std::min(size, size_t(1024));
    size_t hash = 0;
    for (size_t i = 0; i < sampleSize; ++i) hash = hash * 31 + data[i];
    if (size > 2048) {
        for (size_t i = size - sampleSize; i < size; ++i) hash = hash * 31 + data[i];
    }
    ss << "texture_" << hash << "_" << size;
    return ss.str();
}

std::vector<TextureData> ModelLoader::loadMaterialTextures(
    aiMaterial* mat, 
    aiTextureType type, 
    const std::string& typeName, 
    const std::string& directory, 
    const aiScene* scene, 
    const std::string& modelPath) {
    
    std::vector<TextureData> textures;
    std::cout << "Cercando texture di tipo " << typeName << " in " << directory << std::endl;
    
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::cout << "  Texture trovata nel modello: " << str.C_Str() << std::endl;
        
        TextureData texture;
        texture.type = typeName;
        texture.path = str.C_Str();
        bool textureLoaded = false;
        
        // Embedded
        if (str.length > 0 && str.C_Str()[0] == '*') {
            int textureIndex = std::stoi(str.C_Str() + 1);
            std::cout << "  Rilevata texture embedded con indice: " << textureIndex << std::endl;
            
            if (scene && scene->mTextures && textureIndex < (int)scene->mNumTextures) {
                const aiTexture* embeddedTex = scene->mTextures[textureIndex];
                
                std::string contentHash;
                if (embeddedTex->mHeight == 0) {
                    contentHash = generateTextureHash(
                        reinterpret_cast<const unsigned char*>(embeddedTex->pcData),
                        embeddedTex->mWidth
                    );
                } else {
                    size_t dataSize = embeddedTex->mWidth * embeddedTex->mHeight * 4;
                    contentHash = generateTextureHash(
                        reinterpret_cast<const unsigned char*>(embeddedTex->pcData),
                        dataSize
                    );
                }
                
                std::cout << "  Chiave cache texture (hash content): " << contentHash << std::endl;
                
                if (textureCache.find(contentHash) != textureCache.end()) {
                    texture.id = textureCache[contentHash];
                    textureLoaded = true;
                    std::cout << "  Texture embedded trovata nella cache: " << texture.id << std::endl;
                } else {
                    unsigned int textureID;
                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    
                    int width, height, channels;
                    unsigned char* data = nullptr;
                    
                    if (embeddedTex->mHeight == 0) {
                        std::cout << "  Texture embedded compressa (formato: " << embeddedTex->achFormatHint << ")" << std::endl;
                        stbi_set_flip_vertically_on_load(true);
                        data = stbi_load_from_memory(
                            reinterpret_cast<const unsigned char*>(embeddedTex->pcData),
                            embeddedTex->mWidth,
                            &width, &height, &channels, 0);
                        
                        if (data) {
                            GLenum format = (channels == 1) ? GL_RED : (channels == 3) ? GL_RGB : GL_RGBA;
                            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                            glGenerateMipmap(GL_TEXTURE_2D);
                            std::cout << "  Texture embedded caricata: " << width << "x" << height << ", " << channels << " canali" << std::endl;
                            stbi_image_free(data);
                        }
                    } else {
                        width = embeddedTex->mWidth;
                        height = embeddedTex->mHeight;
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, embeddedTex->pcData);
                        glGenerateMipmap(GL_TEXTURE_2D);
                        std::cout << "  Texture embedded raw caricata: " << width << "x" << height << std::endl;
                    }
                    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    
                    texture.id = textureID;
                    textureCache[contentHash] = textureID;
                    textureLoaded = true;
                    std::cout << "  Texture caricata e cached con hash: " << contentHash << " (ID: " << textureID << ")" << std::endl;
                }
            } else {
                std::cerr << "  ERRORE: Impossibile accedere alla texture embedded indice " << textureIndex << std::endl;
                aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
                mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                texture.id = TextureManager::getInstance().createColorTexture(
                    glm::vec4(color.r, color.g, color.b, color.a));
                textureLoaded = true;
            }
        }
        
        // Filesystem
        if (!textureLoaded) {
            std::vector<std::string> possiblePaths;
            possiblePaths.push_back(str.C_Str());
            possiblePaths.push_back(directory + "/" + str.C_Str());
            std::filesystem::path texPath(str.C_Str());
            possiblePaths.push_back(directory + "/" + texPath.filename().string());
            possiblePaths.push_back(directory + "/textures/" + texPath.filename().string());
            possiblePaths.push_back(directory + "/texture/" + texPath.filename().string());
            
            for (const auto& tryPath : possiblePaths) {
                if (std::filesystem::exists(tryPath)) {
                    texture.id = loadTexture(tryPath);
                    if (texture.id != 0) {
                        std::cout << "  Texture caricata da filesystem: " << tryPath << " (ID: " << texture.id << ")" << std::endl;
                        texture.path = tryPath;
                        textureLoaded = true;
                        break;
                    }
                }
            }
        }
        
        if (!textureLoaded) {
            aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                std::cout << "  Usando colore materiale come fallback: (" 
                          << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                texture.id = TextureManager::getInstance().createColorTexture(
                    glm::vec4(color.r, color.g, color.b, color.a));
            } else {
                std::cout << "  Usando texture default" << std::endl;
                texture.id = TextureManager::getInstance().getDefaultTexture();
            }
        }
        
        textures.push_back(texture);
    }
    
    if (textures.empty()) {
        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            std::cout << "  Nessuna texture trovata, creo texture da colore materiale" << std::endl;
            TextureData colorTexture;
            colorTexture.type = typeName;
            colorTexture.path = "generated_color";
            colorTexture.id = TextureManager::getInstance().createColorTexture(
                glm::vec4(color.r, color.g, color.b, color.a));
            textures.push_back(colorTexture);
        }
    }
    
    return textures;
}

void ModelLoader::normalizeModel(std::shared_ptr<ImportedModel> model) {
    auto& meshes = const_cast<std::vector<MeshData>&>(model->getMeshes());
    if (meshes.empty()) return;

    glm::vec3 globalMinAABB(std::numeric_limits<float>::max());
    glm::vec3 globalMaxAABB(std::numeric_limits<float>::lowest());

    for (const auto& mesh : meshes) {
        for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
            globalMinAABB.x = std::min(globalMinAABB.x, mesh.vertices[i]);
            globalMinAABB.y = std::min(globalMinAABB.y, mesh.vertices[i + 1]);
            globalMinAABB.z = std::min(globalMinAABB.z, mesh.vertices[i + 2]);
            globalMaxAABB.x = std::max(globalMaxAABB.x, mesh.vertices[i]);
            globalMaxAABB.y = std::max(globalMaxAABB.y, mesh.vertices[i + 1]);
            globalMaxAABB.z = std::max(globalMaxAABB.z, mesh.vertices[i + 2]);
        }
    }

    glm::vec3 globalCenter = (globalMinAABB + globalMaxAABB) / 2.0f;
    glm::vec3 globalSize = globalMaxAABB - globalMinAABB;
    float maxDim = std::max({ globalSize.x, globalSize.y, globalSize.z });
    if (maxDim < 1e-6f) return;

    float globalScaleFactor = 2.0f / maxDim;

    model->setNormalizationCenter(globalCenter);
    model->setNormalizationScale(globalScaleFactor);

    std::cout << "Normalizzazione globale:" << std::endl;
    std::cout << "  Centro: (" << globalCenter.x << ", " << globalCenter.y << ", " << globalCenter.z << ")" << std::endl;
    std::cout << "  Scala: " << globalScaleFactor << std::endl;
    std::cout << "  AABB originale: " << globalSize.x << " x " << globalSize.y << " x " << globalSize.z << std::endl;

    for (auto& mesh : meshes) {
        for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
            mesh.vertices[i]     = (mesh.vertices[i]     - globalCenter.x) * globalScaleFactor;
            mesh.vertices[i + 1] = (mesh.vertices[i + 1] - globalCenter.y) * globalScaleFactor;
            mesh.vertices[i + 2] = (mesh.vertices[i + 2] - globalCenter.z) * globalScaleFactor;
        }
    }
}

void ImportedModel::setupVertexAttributes() {
    // ImportedModel usa VBO separati impostati in initialize(). Nessuna azione necessaria qui.
}

// Caricamento immagine come plane (usato da openImageFile)
std::shared_ptr<ImagePlaneModel> ModelLoader::loadImageAsPlane(const std::string& path, std::string& errorMessage) {
    std::cout << "Caricamento immagine come plane: " << path << std::endl;

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

    if (!data) {
        errorMessage = "Failed to load image file.\n\nReason: " + std::string(stbi_failure_reason()) + 
                      "\n\nFile: " + path;
        std::cerr << "Errore nel caricamento immagine: " << path << std::endl;
        std::cerr << "stbi_failure_reason: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }

    std::cout << "Immagine caricata: " << width << "x" << height
        << ", componenti: " << nrComponents << std::endl;

    bool hasAlpha = (nrComponents == 4);

    unsigned int textureID;
    glGenTextures(1, &textureID);
    if (textureID == 0) {
        errorMessage = "OpenGL texture creation failed.\n\nThe graphics driver was unable to allocate a texture ID.";
        std::cerr << "Errore: glGenTextures ha fallito" << std::endl;
        stbi_image_free(data);
        return nullptr;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum internalFormat, format;
    if (nrComponents == 1) { internalFormat = GL_R8;    format = GL_RED; }
    else if (nrComponents == 3) { internalFormat = GL_RGB8; format = GL_RGB; }
    else if (nrComponents == 4) { internalFormat = GL_RGBA8; format = GL_RGBA; }
    else {
        errorMessage = "Unsupported image format.\n\nThe image has " + std::to_string(nrComponents) + 
                      " color components, which is not supported.\n\nSupported formats: Grayscale (1), RGB (3), RGBA (4)";
        std::cerr << "Formato immagine non supportato: " << nrComponents << " componenti" << std::endl;
        stbi_image_free(data);
        glDeleteTextures(1, &textureID);
        return nullptr;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Errore OpenGL prima di glTexImage2D: 0x" << std::hex << error << std::dec << std::endl;
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureID);
        return nullptr;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    auto planeModel = std::make_shared<ImagePlaneModel>("ImagePlane");
    planeModel->setTexture(textureID);
    planeModel->setImageSize(width, height, hasAlpha);
    return planeModel;
}

bool ModelLoader::openImageFile(ModelManager& modelManager, SceneManager& sceneManager,
    const glm::vec3& cameraPos, const glm::vec3& cameraTarget,
    bool& objectSelected, bool& showSelectedModelPanel, std::string& errorMessage) {
#ifdef _WIN32
    OPENFILENAME ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(glfwGetCurrentContext());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn)) {
        return false;
    }

    std::string filePath = ofn.lpstrFile;
    auto model = loadImageAsPlane(filePath, errorMessage);
    if (!model) return false;

    // Registra e inizializza
    modelManager.registerModel(model);
    model->initialize();

    // Nota: cameraTarget in questa firma lo usiamo come spawnPos calcolato dal chiamante
    glm::vec3 spawnPos = cameraTarget;

    // Crea oggetto scena
    auto newObj = sceneManager.addObject("ImagePlane", spawnPos);
    if (!newObj) {
        errorMessage = "Failed to create scene object for image plane.";
        return false;
    }

    sceneManager.selectObject(newObj->getId());
    objectSelected = true;
    showSelectedModelPanel = true;
    return true;
#else
    (void)modelManager; (void)sceneManager; (void)cameraPos; (void)cameraTarget; (void)objectSelected; (void)showSelectedModelPanel; (void)errorMessage;
    std::cerr << "openImageFile non implementato per questa piattaforma." << std::endl;
    return false;
#endif
}

bool ModelLoader::openModelFileAdvanced(
    ModelManager& modelManager, 
    SceneManager& sceneManager,
    const glm::vec3& cameraPos, 
    const glm::vec3& cameraTarget,
    bool& objectSelected, 
    bool& showSelectedModelPanel,
    LoadMode mode,
    ProgressCallback progressCallback) {

#ifdef _WIN32
    s_progressCallback = progressCallback;
    reportProgress(0.05f, "Waiting for file selection...");

    OPENFILENAME ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(glfwGetCurrentContext());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Models\0*.obj;*.fbx;*.dae;*.3ds;*.gltf;*.glb\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn)) {
        s_progressCallback = nullptr;
        return false;
    }

    std::string filePath = ofn.lpstrFile;
    reportProgress(0.15f, "Loading model...");
    auto model = loadModel(filePath);
    if (!model) {
        s_progressCallback = nullptr;
        return false;
    }

    reportProgress(0.55f, "Initializing GPU buffers...");
    modelManager.registerModel(model);
    model->initialize();

    glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
        cameraPos, cameraTarget, 0.5f, 1.0f);

    bool success = false;

    if (mode == LoadMode::SINGLE_OBJECT) {
        auto newObj = sceneManager.addObject(model->getName(), spawnPos);
        if (newObj) {
            sceneManager.selectObject(newObj->getId());
            objectSelected = true;
            showSelectedModelPanel = true;
            success = true;
        }
    } else if (mode == LoadMode::SEPARATE_MESHES) {
        std::shared_ptr<SceneObject> firstObject;
        reportProgress(0.70f, "Creating separate mesh objects...");
        createSeparateMeshObjects(model, modelManager, sceneManager, spawnPos, firstObject);
        if (firstObject) {
            sceneManager.selectObject(firstObject->getId());
            objectSelected = true;
            showSelectedModelPanel = true;
            success = true;
        }
    } else { // BY_MATERIAL (non implementato, fallback a SINGLE)
        auto newObj = sceneManager.addObject(model->getName(), spawnPos);
        if (newObj) {
            sceneManager.selectObject(newObj->getId());
            objectSelected = true;
            showSelectedModelPanel = true;
            success = true;
        }
    }

    reportProgress(0.95f, "Finalizing...");
    reportProgress(1.0f, "Done");
    s_progressCallback = nullptr;
    return success;
#else
    (void)modelManager; (void)sceneManager; (void)cameraPos; (void)cameraTarget; (void)objectSelected; (void)showSelectedModelPanel; (void)mode; (void)progressCallback;
    std::cerr << "openModelFileAdvanced non implementato per questa piattaforma." << std::endl;
    return false;
#endif
}