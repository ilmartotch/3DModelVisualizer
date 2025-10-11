#include "../Include/ModelLoader.h"
#include "../Include/TextureManager.h"
#include <iostream>
#include <filesystem>
#include <stb_image.h>
#include <limits>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
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

                // Calcola una posizione di spawn valida usando la logica centralizzata
                // L'offset Y di 1.0f è un valore di default ragionevole per modelli normalizzati.
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
                std::shared_ptr<Model> model = modelManager.getModel(selectedObj->getName());
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
    
    static unsigned int lastTextureID = 0;
    unsigned int currentTextureID = 0;
    
    // Gestione texture
    if (useTexture && textureID != 0) {
        // Usa la texture assegnata al modello
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        currentTextureID = textureID;
    } 
    else if (!textures.empty()) {
        // Usa la prima texture disponibile
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0].id);
        currentTextureID = textures[0].id;
    } 
    else {
        // Usa texture di default
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
        currentTextureID = TextureManager::getInstance().getDefaultTexture();
    }
    
    // Stampa solo se la texture è cambiata
    if (currentTextureID != lastTextureID) {
        std::cout << "Rendering con texture ID: " << currentTextureID << std::endl;
        lastTextureID = currentTextureID;
    }
    
    // Renderizza la mesh
    if (EBO != 0 && !meshes.empty()) {
        // Usa indici se disponibili
        glDrawElements(GL_TRIANGLES, meshes[0].indices.size(), GL_UNSIGNED_INT, 0);
    } else if (!meshes.empty()) {
        // Altrimenti usa vertex array
        glDrawArrays(GL_TRIANGLES, 0, meshes[0].vertices.size() / 3);
    }
    
    // Reimposta lo stato
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0); // Scollega esplicitamente la texture
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

std::shared_ptr<Model> ImportedModel::clone() const {
    auto newModel = std::make_shared<ImportedModel>(getName());
    
    // Copia le mesh e le texture
    for (const auto& mesh : meshes) {
        newModel->addMesh(mesh);
    }
    
    for (const auto& texture : textures) {
        newModel->addTexture(texture);
    }
    
    newModel->setDirectory(directory);
    newModel->setPath(path);
    
    // Se abbiamo una texture, impostiamo la texture ID sul nuovo modello
    if (!textures.empty()) {
        newModel->setTexture(textures[0].id);
    }
    
    return newModel;
}

std::shared_ptr<ImportedModel> ModelLoader::loadModel(const std::string& path) {
    std::cout << "Caricamento modello: " << path << std::endl;
    
    // Crea un nuovo modello
    auto model = std::make_shared<ImportedModel>(std::filesystem::path(path).stem().string());
    
    // Carica il modello con Assimp
    Assimp::Importer importer;
    
    // Aggiungi più post-processing steps per garantire migliore qualità
    unsigned int flags = aiProcess_Triangulate | 
                         aiProcess_GenSmoothNormals |
                         aiProcess_CalcTangentSpace |
                         aiProcess_JoinIdenticalVertices |
                         aiProcess_SortByPType;
    
    // Non invertire UVs in Assimp, lo gestiremo nel caricamento texture
    const aiScene* scene = importer.ReadFile(path, flags);
    
    // Verifica che il caricamento sia riuscito
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }
    
    // Estrai la directory dal percorso
    std::string directory;
    std::filesystem::path filePath(path);
    directory = filePath.parent_path().string();
    
    std::cout << "Directory del modello: " << directory << std::endl;
    model->setDirectory(directory);
    model->setPath(path);
    
    // Processa i nodi del modello
    processNode(scene->mRootNode, scene, model, directory);

    // Normalizza il modello dopo aver caricato tutti i dati
    normalizeModel(model);
    
    // Se non ci sono texture, assegna una texture di default
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
        std::cout << "Texture caricata: " << path << " (" << width << "x" << height 
                  << ", componenti: " << nrComponents << ")" << std::endl;
        
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
        std::cerr << "stbi_failure_reason: " << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
        return 0;
    }
    
    // Aggiungi la texture alla cache
    textureCache[path] = textureID;
    
    return textureID;
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene, 
                            std::shared_ptr<ImportedModel> model, const std::string& directory) {
    std::cout << "Processando nodo: " << node->mName.C_Str() << " con " 
              << node->mNumMeshes << " mesh e " << node->mNumChildren << " figli" << std::endl;
    
    // Processa tutte le mesh del nodo
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::cout << "Processando mesh " << i << ": " << mesh->mName.C_Str() 
                  << " con indice materiale " << mesh->mMaterialIndex << std::endl;
        
        model->addMesh(processMesh(mesh, scene));
        
        // Carica i materiali per la mesh
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::cout << "Materiale trovato: " << material->GetName().C_Str() << std::endl;
            
            // Passa il path del modello
            std::vector<TextureData> diffuseMaps = loadMaterialTextures(
                material, aiTextureType_DIFFUSE, "texture_diffuse", directory, scene, model->getPath());
            
            std::cout << "Caricate " << diffuseMaps.size() << " texture diffuse" << std::endl;
            
            // Aggiungi le texture al modello
            for (const auto& texture : diffuseMaps) {
                model->addTexture(texture);
                if (texture.type == "texture_diffuse" && !model->hasTexture()) {
                    model->setTexture(texture.id);
                    std::cout << "Texture principale impostata: ID=" << texture.id << std::endl;
                }
            }
            
            // Passa il path del modello
            std::vector<TextureData> specularMaps = loadMaterialTextures(
                material, aiTextureType_SPECULAR, "texture_specular", directory, scene, model->getPath());
            
            std::cout << "Caricate " << specularMaps.size() << " texture speculari" << std::endl;
            
            // Aggiungi le texture al modello
            for (const auto& texture : specularMaps) {
                model->addTexture(texture);
            }
            
            // Se non abbiamo trovato texture, prova a recuperare il colore del materiale
            if (diffuseMaps.empty() && specularMaps.empty()) {
                aiColor3D color(0.f, 0.f, 0.f);
                if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                    std::cout << "Nessuna texture trovata, uso colore del materiale: (" 
                              << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                    
                    // Crea una texture dal colore
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
    
    // Processa i nodi figli in modo ricorsivo
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model, directory);
    }
}

MeshData ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene) {
    MeshData meshData;
    
    std::cout << "Elaborando mesh con " << mesh->mNumVertices << " vertici" << std::endl;
    
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
            // Un vertice può avere fino a 8 set di coordinate texture
            // Prendiamo solo il primo set (0)
            meshData.texCoords.push_back(mesh->mTextureCoords[0][i].x);
            meshData.texCoords.push_back(mesh->mTextureCoords[0][i].y);
            
            // Debug: stampa coordinate texture per un campione di vertici
            if (i < 5 || i > mesh->mNumVertices - 5) {
                std::cout << "  TexCoord[" << i << "]: (" 
                          << mesh->mTextureCoords[0][i].x << ", " 
                          << mesh->mTextureCoords[0][i].y << ")" << std::endl;
            }
        } else {
            // Se la mesh non ha coordinate texture, aggiungi coordinate di default
            meshData.texCoords.push_back(0.0f);
            meshData.texCoords.push_back(0.0f);
        }
    }
    
    // Processa indici
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            meshData.indices.push_back(face.mIndices[j]);
        }
    }
    
    // Aggiungi materiale
    if (mesh->mMaterialIndex >= 0) {
        meshData.materialName = "material_" + std::to_string(mesh->mMaterialIndex);
    }
    
    std::cout << "Mesh elaborata con " << meshData.vertices.size() / 3 << " vertici, " 
              << meshData.texCoords.size() / 2 << " coordinate UV, e " 
              << meshData.indices.size() << " indici" << std::endl;
    
    return meshData;
}

std::vector<TextureData> ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, 
                                                       const std::string& typeName, const std::string& directory, 
                                                       const aiScene* scene, const std::string& modelPath) {
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
        
        // Gestione per texture embedded (iniziano con *)
        if (str.length > 0 && str.C_Str()[0] == '*') {
            // Estrai l'indice della texture embedded
            int textureIndex = std::stoi(str.C_Str() + 1);
            std::cout << "  Rilevata texture embedded con indice: " << textureIndex << std::endl;
            
            // Usa il path COMPLETO del modello per la chiave univoca
            std::string uniqueKey = modelPath + "_embedded_" + std::to_string(textureIndex);
            std::cout << "  Chiave cache texture: " << uniqueKey << std::endl;
            
            // Verifica se è già nella cache
            if (textureCache.find(uniqueKey) != textureCache.end()) {
                texture.id = textureCache[uniqueKey];
                textureLoaded = true;
                std::cout << "  Texture embedded trovata nella cache: " << texture.id << std::endl;
            } 
            else if (scene && scene->mTextures && textureIndex < scene->mNumTextures) {
                // Ottieni la texture embedded direttamente dalla scena
                const aiTexture* embeddedTex = scene->mTextures[textureIndex];
                
                unsigned int textureID;
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                
                // Determina il formato della texture
                GLenum format;
                int width, height, channels;
                unsigned char* data = nullptr;
                
                if (embeddedTex->mHeight == 0) {
                    // Texture compressa (i dati sono già in un formato immagine come png/jpg)
                    std::cout << "  Texture embedded compressa (formato: " << embeddedTex->mFilename.C_Str() << ")" << std::endl;
                    
                    // Carica i dati usando stb_image
                    stbi_set_flip_vertically_on_load(true);
                    data = stbi_load_from_memory(
                        reinterpret_cast<const unsigned char*>(embeddedTex->pcData),
                        embeddedTex->mWidth,
                        &width, &height, &channels, 0);
                    
                    if (data) {
                        if (channels == 1) format = GL_RED;
                        else if (channels == 3) format = GL_RGB;
                        else if (channels == 4) format = GL_RGBA;
                        else format = GL_RGB;
                        
                        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                        
                        std::cout << "  Texture embedded caricata: " << width << "x" << height << ", " << channels << " canali" << std::endl;
                        
                        stbi_image_free(data);
                    }
                }
                else {
                    // Texture non compressa (raw data)
                    width = embeddedTex->mWidth;
                    height = embeddedTex->mHeight;
                    
                    // Assume formato RGBA per texture raw (non compressa)
                    format = GL_RGBA;
                    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, embeddedTex->pcData);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    
                    std::cout << "  Texture embedded raw caricata: " << width << "x" << height << std::endl;
                }
                
                // Parametri texture
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                texture.id = textureID;
                textureCache[uniqueKey] = textureID;
                textureLoaded = true;
            }
            else {
                // Creazione di una texture colorata sostitutiva come ultima risorsa
                aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
                
                // Prova a ottenere il colore diffuso dal materiale
                if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                    std::cout << "  Usando colore diffuso dal materiale: (" 
                              << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                }
                
                texture.id = TextureManager::getInstance().createColorTexture(
                    glm::vec4(color.r, color.g, color.b, color.a));
                
                std::cout << "  Creata texture colorata sostitutiva per texture embedded: " << texture.id << std::endl;
                
                textureCache[uniqueKey] = texture.id;
                textureLoaded = true;
            }
        }
        
        // Se non è una texture embedded o non è stata caricata, prova i percorsi nel filesystem
        if (!textureLoaded) {
            // Prova diverse possibili posizioni per la texture
            std::vector<std::string> possiblePaths;
            
            // 1. Percorso completo come specificato
            possiblePaths.push_back(str.C_Str());
            
            // 2. Relativo alla directory del modello
            possiblePaths.push_back(directory + "/" + str.C_Str());
            
            // 3. Solo il nome del file nella directory del modello
            std::filesystem::path texPath(str.C_Str());
            possiblePaths.push_back(directory + "/" + texPath.filename().string());
            
            // 4. Cartelle texture/textures nella directory del modello
            possiblePaths.push_back(directory + "/textures/" + texPath.filename().string());
            possiblePaths.push_back(directory + "/texture/" + texPath.filename().string());
            
            // Prova tutti i percorsi possibili
            for (const auto& tryPath : possiblePaths) {
                std::cout << "  Tentativo di caricamento da: " << tryPath << std::endl;
                
                if (std::filesystem::exists(tryPath)) {
                    texture.id = loadTexture(tryPath);
                    if (texture.id != 0) {
                        std::cout << "  Texture caricata con successo da " << tryPath << " (ID: " << texture.id << ")" << std::endl;
                        texture.path = tryPath;
                        textureLoaded = true;
                        break;
                    }
                }
            }
        }
        
        if (!textureLoaded) {
            // Se non abbiamo trovato texture, prova a recuperare il colore del materiale
            aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                std::cout << "  ATTENZIONE: Usando colore del materiale: (" 
                          << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                texture.id = TextureManager::getInstance().createColorTexture(
                    glm::vec4(color.r, color.g, color.b, color.a));
            }
            else {
                std::cout << "  ATTENZIONE: Impossibile trovare la texture! Uso texture di default" << std::endl;
                texture.id = TextureManager::getInstance().getDefaultTexture();
            }
        }
        
        textures.push_back(texture);
    }
    
    // Se non ci sono texture associate ma c'è un colore diffuso nel materiale
    if (textures.empty()) {
        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            std::cout << "  Nessuna texture trovata, uso colore del materiale: (" 
                    << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
            
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
    glm::vec3 minAABB(std::numeric_limits<float>::max());
    glm::vec3 maxAABB(std::numeric_limits<float>::lowest());

    // Ottieni un riferimento non-const alle mesh per poterle modificare
    auto& meshes = const_cast<std::vector<MeshData>&>(model->getMeshes());
    if (meshes.empty()) {
        return; // Nessun dato da normalizzare
    }

    // 1. Calcola il Bounding Box (AABB) complessivo
    for (const auto& mesh : meshes) {
        for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
            minAABB.x = std::min(minAABB.x, mesh.vertices[i]);
            minAABB.y = std::min(minAABB.y, mesh.vertices[i + 1]);
            minAABB.z = std::min(minAABB.z, mesh.vertices[i + 2]);
            maxAABB.x = std::max(maxAABB.x, mesh.vertices[i]);
            maxAABB.y = std::max(maxAABB.y, mesh.vertices[i + 1]);
            maxAABB.z = std::max(maxAABB.z, mesh.vertices[i + 2]);
        }
    }

    // 2. Calcola il centro e il fattore di scala
    glm::vec3 center = (minAABB + maxAABB) / 2.0f;
    glm::vec3 size = maxAABB - minAABB;
    float maxDim = std::max({ size.x, size.y, size.z });
    
    // Evita divisione per zero se il modello è un punto
    if (maxDim < 1e-6) {
        return;
    }

    float scaleFactor = 2.0f / maxDim; // Normalizza a una dimensione di 2 unità

    // 3. Applica la trasformazione a tutti i vertici
    for (auto& mesh : meshes) {
        for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
            // Centra il vertice
            mesh.vertices[i] -= center.x;
            mesh.vertices[i + 1] -= center.y;
            mesh.vertices[i + 2] -= center.z;

            // Scala il vertice
            mesh.vertices[i] *= scaleFactor;
            mesh.vertices[i + 1] *= scaleFactor;
            mesh.vertices[i + 2] *= scaleFactor;
        }
    }
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