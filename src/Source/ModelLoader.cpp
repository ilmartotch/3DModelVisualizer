#include "../Include/ModelLoader.h"
#include <iostream>
#include <filesystem>
#include <stb_image.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>
#endif

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

                // Calcola una posizione di spawn valida
                glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
                    cameraPos, cameraTarget, 0.0f, 1.0f);

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
#endif
}

bool ModelLoader::openTextureFile(ModelManager& modelManager, SceneManager& sceneManager) {
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

        // Se c'è un oggetto selezionato, applica la texture a quell'oggetto
        auto selectedObj = sceneManager.getSelectedObject();
        if (selectedObj) {
            // Carica la texture
            GLuint textureID = loadTexture(filePath);
            if (textureID) {
                // Ottieni il modello dell'oggetto selezionato e imposta la sua texture
                std::shared_ptr<Model> model = modelManager.getModel(selectedObj->getModelName());
                if (model) {
                    model->setTexture(textureID);
                    model->getTextureID();
                    // Notifica all'utente
                    std::cout << "Texture applicata con successo a " << selectedObj->getName() << std::endl;
                    return true;
                }
            }
            else {
                std::cerr << "Errore nel caricamento della texture: " << filePath << std::endl;
            }
        }
        else {
            // Nessun oggetto selezionato, mostra un messaggio
            std::cerr << "Seleziona un oggetto prima di applicare una texture." << std::endl;
        }
    }
    return false;
#endif
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
        std::cerr << "Error: No mesh data available for model: " << getName() << std::endl;
        return;
    }
    
    // Crea il VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Per ogni mesh, crea i buffer necessari
    for (const auto& mesh : meshes) {
        GLuint vbo_vertices, vbo_normals, vbo_texcoords;
        
        // VBO per i vertici
        glGenBuffers(1, &vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), 
                   mesh.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
        
        // VBO per le normali, se disponibili
        if (!mesh.normals.empty()) {
            glGenBuffers(1, &vbo_normals);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
            glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(float), 
                       mesh.normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);
            vbos.push_back(vbo_normals);
        }
        
        // VBO per le coordinate texture, se disponibili
        if (!mesh.texCoords.empty()) {
            glGenBuffers(1, &vbo_texcoords);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
            glBufferData(GL_ARRAY_BUFFER, mesh.texCoords.size() * sizeof(float), 
                       mesh.texCoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);
            vbos.push_back(vbo_texcoords);
        }
        
        // EBO per gli indici
        if (!mesh.indices.empty()) {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int),
                       mesh.indices.data(), GL_STATIC_DRAW);
        }
        
        vbos.push_back(vbo_vertices);
    }
    
    glBindVertexArray(0);
    m_initialized = true;
}

void ImportedModel::render() {
    if (!m_initialized) {
        std::cerr << "Model not initialized: " << getName() << std::endl;
        return;
    }
    
    glBindVertexArray(VAO);
    
    // Applica le texture
    for (unsigned int i = 0; i < textures.size(); i++) {
        // Attiva la texture nell'unità appropriata
        glActiveTexture(GL_TEXTURE0 + i);
        
        // Ottieni il nome dell'uniforme shader (es. "texture_diffuse1")
        std::string number;
        std::string name = textures[i].type;
        
        // Bind della texture
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    
    // Renderizza la mesh
    if (EBO != 0) {
        // Usa indici se disponibili
        glDrawElements(GL_TRIANGLES, meshes[0].indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        // Altrimenti usa vertex array
        glDrawArrays(GL_TRIANGLES, 0, meshes[0].vertices.size() / 3);
    }
    
    // Reimposta lo stato
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void ImportedModel::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    
    for (auto& vbo : vbos) {
        glDeleteBuffers(1, &vbo);
    }
    vbos.clear();
    
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    
    // Elimina le texture
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

std::shared_ptr<ImportedModel> ModelLoader::loadModel(const std::string& path) {
    // Crea un nuovo modello
    auto model = std::make_shared<ImportedModel>(std::filesystem::path(path).stem().string());
    
    // Carica il modello con Assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals);
    
    // Verifica che il caricamento sia riuscito
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }
    
    // Estrai la directory dal percorso
    std::string directory = path.substr(0, path.find_last_of('/'));
    model->setDirectory(directory);
    
    // Processa i nodi del modello
    processNode(scene->mRootNode, scene, model, directory);
    
    return model;
}

unsigned int ModelLoader::loadTexture(const std::string& path) {
    // Verifica se la texture è già stata caricata
    if (textureCache.find(path) != textureCache.end()) {
        return textureCache[path];
    }
    
    // Crea una nuova texture
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    // Carica i dati dell'immagine con stb_image
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    
    if (data) {
        GLenum format = 0;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Imposta parametri texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
    
    // Aggiungi la texture alla cache
    textureCache[path] = textureID;
    
    return textureID;
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene, 
                            std::shared_ptr<ImportedModel> model, const std::string& directory) {
    // Processa tutte le mesh del nodo
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model->addMesh(processMesh(mesh, scene));
        
        // Carica i materiali per la mesh
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            
            // Carica le texture diffuse
            std::vector<TextureData> diffuseMaps = loadMaterialTextures(
                material, aiTextureType_DIFFUSE, "texture_diffuse", directory);
            
            // Aggiungi le texture al modello
            for (const auto& texture : diffuseMaps) {
                model->addTexture(texture);
            }
            
            // Carica le texture specular
            std::vector<TextureData> specularMaps = loadMaterialTextures(
                material, aiTextureType_SPECULAR, "texture_specular", directory);
            
            // Aggiungi le texture al modello
            for (const auto& texture : specularMaps) {
                model->addTexture(texture);
            }
        }
    }
    
    // Processa i nodi figli
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model, directory);
    }
}

MeshData ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene) {
    MeshData meshData;
    
    // Processa vertici
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Posizione
        meshData.vertices.push_back(mesh->mVertices[i].x);
        meshData.vertices.push_back(mesh->mVertices[i].y);
        meshData.vertices.push_back(mesh->mVertices[i].z);
        
        // Normali
        if (mesh->HasNormals()) {
            meshData.normals.push_back(mesh->mNormals[i].x);
            meshData.normals.push_back(mesh->mNormals[i].y);
            meshData.normals.push_back(mesh->mNormals[i].z);
        }
        
        // Coordinate texture
        if (mesh->mTextureCoords[0]) {
            meshData.texCoords.push_back(mesh->mTextureCoords[0][i].x);
            meshData.texCoords.push_back(mesh->mTextureCoords[0][i].y);
        }
    }
    
    // Processa indici
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            meshData.indices.push_back(face.mIndices[j]);
    }
    
    // Aggiungi materiale
    if (mesh->mMaterialIndex >= 0) {
        meshData.materialName = "material_" + std::to_string(mesh->mMaterialIndex);
    }
    
    return meshData;
}

std::vector<TextureData> ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, 
                                                       const std::string& typeName, const std::string& directory) {
    std::vector<TextureData> textures;
    
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        
        TextureData texture;
        std::string texturePath = directory + '/' + std::string(str.C_Str());
        
        // Carica la texture
        texture.id = loadTexture(texturePath);
        texture.type = typeName;
        texture.path = texturePath;
        textures.push_back(texture);
    }
    
    return textures;
}

void ImportedModel::setupVertexAttributes(){
    glBindVertexArray(VAO);

    // Posizione
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Normale
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Coordinate texture
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}