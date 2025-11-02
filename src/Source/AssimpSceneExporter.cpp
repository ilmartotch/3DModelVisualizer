#include "../Include/AssimpSceneExporter.h"
#include "../Include/SceneManager.h"
#include "../Include/ModelManager.h"
#include "../Include/SceneObject.h"
#include "../Include/Model.h"
#include <iostream>
#include <chrono>
#include <assimp/Importer.hpp>
#include <set>
#include <unordered_set>
#include <fstream>
#include <algorithm>

std::vector<AssimpSceneExporter::ExportFormat> AssimpSceneExporter::getSupportedFormats() {
    return {
        // Wavefront OBJ - universale
        {
            "obj",
            "Wavefront OBJ",
            ".obj",
            "Formato universale, supportato ovunque. Leggero e leggibile.",
            "Uso generale, web, visualizzatori 3D, editing",
            true,   // Materials (.mtl)
            true,   // Textures
            false,  // No animations
            false,  // Testo
            false   // Embedded textures NON supportate
        },
        
        // glTF 2.0 - Standard moderno
        {
            "gltf2",
            "glTF 2.0",
            ".gltf",
            "Standard moderno per il web. Ottimo per WebGL e Three.js.",
            "Web 3D, AR/VR, pipeline moderne",
            true,   // Materials (PBR)
            true,   // Textures
            true,   // Animations
            false,  // JSON + bin
            true    // Embedded supportato (data URI/BUFFERS)
        },
        
        // glTF Binary
        {
            "glb2",
            "glTF 2.0 Binary",
            ".glb",
            "Come glTF ma in un unico file binario. Più compatto.",
            "Distribuzione, app mobile, AR/VR",
            true,
            true,
            true,
            true,   // Binario
            true    // Embedded supportato (file unico)
        },
        
        // Autodesk FBX
        {
            "fbx",
            "Autodesk FBX",
            ".fbx",
            "Standard industry per Maya, 3ds Max, Blender, Unity, Unreal.",
            "Game development, animazione, VFX",
            true,
            true,
            true,
            true,   // Binario
            true    // Embedded generalmente supportato
        },
        
        // COLLADA
        {
            "collada",
            "COLLADA",
            ".dae",
            "Formato XML aperto. Buona interoperabilità tra software.",
            "Interchange tra DCC tools diversi",
            true,
            true,
            true,
            false,  // XML
            true    // Embedded possibile (data URI/base64)
        },
        
        // STL - Stampa 3D
        {
            "stl",
            "STereoLithography",
            ".stl",
            "Solo geometria. Usato per stampa 3D e CAD.",
            "Stampa 3D, prototipazione, CAD",
            false,  // No materials
            false,  // No textures
            false,
            true,   // Binario
            false   // No embedded
        },
        
        // PLY - Stanford
        {
            "ply",
            "Stanford PLY",
            ".ply",
            "Formato per mesh con colori vertex. Usato in scansioni 3D.",
            "Scansioni 3D, point clouds, ricerca",
            false,
            false,
            false,
            true,   // Binario possibile
            false   // No embedded textures
        }
    };
}

AssimpSceneExporter::ExportPreview AssimpSceneExporter::generateExportPreview(
    const SceneManager& sceneManager,
    const ModelManager& modelManager,
    bool copyTextures)
{
    (void)modelManager; // non necessario per il preview

    ExportPreview preview;

    const auto& objects = sceneManager.getObjects();
    preview.totalObjects = static_cast<unsigned int>(objects.size());

    std::set<std::string> uniqueTexturePaths;

    for (const auto& obj : objects) {
        if (!obj) continue;

        // Conteggi base
        auto model = obj->getModel();
        if (model) {
            const auto& vertices = model->getVertices();
            const auto& indices = model->getIndices();

            const bool hasUVs = model->hasUVCoordinates();
            const size_t floatsPerVertex = hasUVs ? 8u : 6u;
            if (floatsPerVertex > 0 && (vertices.size() % floatsPerVertex) == 0) {
                preview.totalVertices += static_cast<unsigned int>(vertices.size() / floatsPerVertex);
            }

            if (!indices.empty()) {
                preview.totalFaces += static_cast<unsigned int>(indices.size() / 3u);
            }
        }

        // Percorso texture con priorità
        std::string texturePath;
        if (obj->hasOverrideTexture()) {
            texturePath = obj->getOverrideTextureSourcePath();
        } else if (obj->getModel() && obj->getModel()->hasTexture()) {
            texturePath = obj->getModel()->getTextureSourcePath();
        }

        if (!texturePath.empty()) {
            TextureInfo info;
            info.objectName = obj->getName();
            info.texturePath = texturePath;
            info.textureFilename = fs::path(texturePath).filename().string();
            info.exists = fs::exists(texturePath);
            info.fileSize = info.exists ? static_cast<size_t>(fs::file_size(texturePath)) : 0;
            info.willBeCopied = copyTextures;

            preview.textures.push_back(info);

            if (info.exists) {
                preview.texturesFound++;
                preview.estimatedTotalSize += info.fileSize;
                uniqueTexturePaths.insert(texturePath);
            } else {
                preview.texturesMissing++;
            }
        }
    }

    preview.texturesUnique = static_cast<int>(uniqueTexturePaths.size());
    return preview;
}

AssimpSceneExporter::ExportResult AssimpSceneExporter::exportScene(
    const SceneManager& sceneManager,
    const ModelManager& modelManager,
    const std::string& outputPath,
    const std::string& formatId,
    bool copyTextures,
    bool embedTextures)
{
    (void)modelManager;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    ExportResult result;
    result.outputPath = outputPath;
    
    std::cout << "[ASSIMP EXPORT] Inizio export formato: " << formatId << std::endl;
    std::cout << "[ASSIMP EXPORT] Output: " << outputPath << std::endl;
    
    // Costruisci aiScene da SceneManager
    aiScene* scene = buildAssimpScene(sceneManager, result);
    if (!scene) {
        result.errorMessage = "Impossibile costruire la scena Assimp";
        std::cerr << "[ASSIMP EXPORT] " << result.errorMessage << std::endl;
        return result;
    }
    
    std::cout << "[ASSIMP EXPORT] Scena creata: " 
              << scene->mNumMeshes << " meshes, "
              << scene->mNumMaterials << " materials" << std::endl;

    // Determina se il formato supporta embedded textures
    const auto formats = getSupportedFormats();
    auto fmtIt = std::find_if(formats.begin(), formats.end(), [&](const ExportFormat& f){ return f.id == formatId; });
    const bool supportsEmbedded = (fmtIt != formats.end()) ? fmtIt->supportsEmbeddedTextures : false;
    const bool supportsTextures  = (fmtIt != formats.end()) ? fmtIt->supportsTextures : false;

    // Crea mappa indice-materiale -> percorso texture sorgente
    const auto textureMap = buildTexturePathMap(sceneManager);

    // Gestione texture: embed se richiesto e supportato, altrimenti copia esterna (se richiesto)
    if (supportsTextures) {
        if (embedTextures && supportsEmbedded) {
            embedTexturesIntoScene(scene, textureMap, result);
        } else {
            // Se embedding non supportato ma richiesto, segnala all'utente
            if (embedTextures && !supportsEmbedded) {
                result.warnings.push_back("Il formato selezionato non supporta l'incorporamento delle texture. Verranno salvate in /textures accanto al file.");
            }

            if (copyTextures) {
                fs::path exportDir = fs::path(outputPath).parent_path();
                std::string copyError;
                if (!copyTexturesWithTracking(textureMap, exportDir, copyError)) {
                    std::cerr << "[ASSIMP EXPORT] Warning copia texture: " << copyError << std::endl;
                    result.warnings.push_back(std::string("Copia texture: ") + copyError);
                } else {
                    // Aggiorna i materiali della scena a percorsi relativi ("textures/filename")
                    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
                        auto it2 = textureMap.find(i);
                        if (it2 == textureMap.end()) continue;

                        const fs::path srcPath(it2->second);
                        if (!fs::exists(srcPath)) continue;
                        const std::string relativePath = std::string("textures/") + srcPath.filename().string();
                        aiString newPath(relativePath.c_str());
                        scene->mMaterials[i]->AddProperty(&newPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
                        result.texturesCopiedCount++;
                    }
                    result.externalTexturesCopied = (result.texturesCopiedCount > 0);
                }
            }
        }

        // Conta texture mancanti (path vuoto o file assente)
        for (const auto& kv : textureMap) {
            const std::string& p = kv.second;
            if (p.empty() || !fs::exists(p)) {
                result.texturesMissing++;
            }
        }
        if (result.texturesMissing > 0) {
            result.warnings.push_back("Alcune texture non sono state salvate perché il file di origine non è disponibile.");
        }
    }
    
    // Export con Assimp Exporter
    Assimp::Exporter exporter;
    
    // Post-processing flags
    unsigned int exportFlags = 
        aiProcess_Triangulate |           // Assicura triangoli
        aiProcess_JoinIdenticalVertices | // Ottimizza
        aiProcess_SortByPType;            // Ordina per tipo primitiva
    
    aiReturn exportResult = exporter.Export(
        scene,
        formatId,
        outputPath,
        exportFlags
    );
    
    if (exportResult != AI_SUCCESS) {
        result.success = false;
        result.errorMessage = std::string("Export fallito: ") + exporter.GetErrorString();
        std::cerr << "[ASSIMP EXPORT] " << result.errorMessage << std::endl;
        delete scene;
        return result;
    }
    
    // Calcola statistiche
    calculateSceneStats(scene, result);
    
    // Dimensione file
    try {
        if (fs::exists(outputPath)) {
            result.fileSize = fs::file_size(outputPath);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ASSIMP EXPORT] Impossibile leggere dimensione file: " << e.what() << std::endl;
    }
    
    // Tempo export
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = endTime - startTime;
    result.exportTimeSeconds = duration.count();
    
    result.success = true;
    
    std::cout << "[ASSIMP EXPORT]   Export completato con successo!" << std::endl;
    std::cout << "[ASSIMP EXPORT]   Vertici: " << result.totalVertices << std::endl;
    std::cout << "[ASSIMP EXPORT]   Facce: " << result.totalFaces << std::endl;
    std::cout << "[ASSIMP EXPORT]   Dimensione: " << (result.fileSize / 1024) << " KB" << std::endl;
    if (result.texturesEmbedded) {
        std::cout << "[ASSIMP EXPORT]   Texture embedded: " << result.texturesEmbeddedCount << std::endl;
    }
    if (result.externalTexturesCopied) {
        std::cout << "[ASSIMP EXPORT]   Texture copiate: " << result.texturesCopiedCount << std::endl;
    }
    if (result.texturesMissing > 0) {
        std::cout << "[ASSIMP EXPORT]   Texture mancanti: " << result.texturesMissing << std::endl;
    }
    
    delete scene;
    return result;
}

std::unordered_map<unsigned int, std::string> AssimpSceneExporter::buildTexturePathMap(
    const SceneManager& sceneManager
) {
    std::unordered_map<unsigned int, std::string> map;

    const auto& objects = sceneManager.getObjects();
    unsigned int materialIndex = 0;

    for (const auto& obj : objects) {
        if (!obj) { ++materialIndex; continue; }

        std::string texturePath;
        if (obj->hasOverrideTexture()) {
            texturePath = obj->getOverrideTextureSourcePath();
        } else if (obj->getModel() && obj->getModel()->hasTexture()) {
            texturePath = obj->getModel()->getTextureSourcePath();
        }

        if (!texturePath.empty()) {
            map.emplace(materialIndex, texturePath);
        }

        ++materialIndex;
    }

    return map;
}

aiScene* AssimpSceneExporter::buildAssimpScene(
    const SceneManager& sceneManager,
    ExportResult& result)
{
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode("SceneRoot");
    
    const auto& objects = sceneManager.getObjects();
    const size_t numObjects = objects.size();
    if (numObjects == 0) {
        std::cerr << "[ASSIMP EXPORT] Warning: Scena vuota" << std::endl;
        return scene;
    }
    
    // Alloca array
    scene->mNumMeshes = static_cast<unsigned int>(numObjects);
    scene->mMeshes = new aiMesh*[numObjects];
    
    scene->mNumMaterials = static_cast<unsigned int>(numObjects);
    scene->mMaterials = new aiMaterial*[numObjects];
    
    // Alloca children del root node
    scene->mRootNode->mNumChildren = static_cast<unsigned int>(numObjects);
    scene->mRootNode->mChildren = new aiNode*[numObjects];
    
    unsigned int meshIndex = 0;
    unsigned int vertexOffset = 0;

    const unsigned int capacityMeshes    = scene->mNumMeshes;
    const unsigned int capacityMaterials = scene->mNumMaterials;
    const unsigned int capacityChildren  = scene->mRootNode->mNumChildren;

    for (const auto& obj : objects) {
        if (!obj || !obj->getModel()) {
            std::cerr << "[ASSIMP EXPORT] Warning: Oggetto nullo saltato" << std::endl;
            continue;
        }

        // Converti Model → aiMesh
        aiMesh* mesh = convertModelToMesh(
            obj->getModel().get(),
            obj->getName(),
            vertexOffset
        );

        if (!mesh) {
            std::cerr << "[ASSIMP EXPORT] Warning: Impossibile convertire mesh: "
                      << obj->getName() << std::endl;
            continue;
        }

        if (meshIndex >= capacityMeshes || meshIndex >= capacityMaterials || meshIndex >= capacityChildren) {
            std::cerr << "[ASSIMP EXPORT] Bound overflow prevenuto (meshIndex=" << meshIndex
                      << ", capacities: meshes=" << capacityMeshes
                      << ", materials=" << capacityMaterials
                      << ", children=" << capacityChildren << ")" << std::endl;
            delete mesh;
            break;
        }

        mesh->mMaterialIndex = meshIndex;
        scene->mMeshes[meshIndex] = mesh;

        scene->mMaterials[meshIndex] = createMaterialFromObject(
            obj.get(),
            obj->getName() + std::string("_Mat")
        );

        // Crea node figlio
        aiNode* node = new aiNode(obj->getName());
        node->mParent = scene->mRootNode;

        // Applica trasformazione
        node->mTransformation = glmToAiMatrix(obj->getModelMatrix());

        // Associa mesh
        node->mNumMeshes = 1;
        node->mMeshes = new unsigned int[1];
        node->mMeshes[0] = meshIndex;

        scene->mRootNode->mChildren[meshIndex] = node;

        vertexOffset += mesh->mNumVertices;
        meshIndex++;
    }

    // Allinea i contatori al numero effettivo di elementi popolati
    scene->mNumMeshes = meshIndex;
    scene->mNumMaterials = meshIndex;
    scene->mRootNode->mNumChildren = meshIndex;

    result.totalMeshes = meshIndex;
    return scene;
}

aiMesh* AssimpSceneExporter::convertModelToMesh(
    const Model* model,
    const std::string& meshName,
    unsigned int& vertexOffset)
{
    (void)vertexOffset;

    if (!model || !model->isInitialized()) {
        return nullptr;
    }

    const std::vector<float>& vertices = model->getVertices();
    const std::vector<unsigned int>& indices = model->getIndices();

    const bool hasUVs = model->hasUVCoordinates();
    const size_t floatsPerVertex = hasUVs ? 8u : 6u;

    if (vertices.empty() || (vertices.size() % floatsPerVertex) != 0) {
        std::cerr << "[ASSIMP EXPORT] Warning: Vertices non coerenti per mesh: " << meshName << std::endl;
        return nullptr;
    }

    const size_t numVertices = vertices.size() / floatsPerVertex;
    if (numVertices == 0) {
        std::cerr << "[ASSIMP EXPORT] Warning: Mesh senza vertici: " << meshName << std::endl;
        return nullptr;
    }
    
    aiMesh* mesh = new aiMesh();
    mesh->mName = aiString(meshName);
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    
    // Vertici
    mesh->mNumVertices = static_cast<unsigned int>(numVertices);
    mesh->mVertices = new aiVector3D[numVertices];
    mesh->mNormals = new aiVector3D[numVertices];

    if (hasUVs) {
        mesh->mTextureCoords[0] = new aiVector3D[numVertices];
        mesh->mNumUVComponents[0] = 2;
    }

    for (size_t i = 0; i < numVertices; i++) {
        const size_t idx = i * floatsPerVertex;

        // Position
        mesh->mVertices[i] = aiVector3D(
            vertices[idx + 0],
            vertices[idx + 1],
            vertices[idx + 2]
        );
        
        // Normal
        mesh->mNormals[i] = aiVector3D(
            vertices[idx + 3],
            vertices[idx + 4],
            vertices[idx + 5]
        );
        
        // UV
        if (hasUVs) {
            mesh->mTextureCoords[0][i] = aiVector3D(
                vertices[idx + 6],
                vertices[idx + 7],
                0.0f
            );
        }
    }
    
    // Facce (triangoli)
    if (!indices.empty()) {
        if ((indices.size() % 3u) != 0) {
            std::cerr << "[ASSIMP EXPORT] Warning: Indici non multipli di 3 per mesh: " << meshName << std::endl;
            return mesh;
        }

        mesh->mNumFaces = static_cast<unsigned int>(indices.size() / 3u);
        mesh->mFaces = new aiFace[mesh->mNumFaces];

        for (size_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];
            face.mIndices[0] = indices[i * 3 + 0];
            face.mIndices[1] = indices[i * 3 + 1];
            face.mIndices[2] = indices[i * 3 + 2];
        }
    }

    return mesh;
}

aiMaterial* AssimpSceneExporter::createMaterialFromObject(
    const SceneObject* obj,
    const std::string& materialName,
    const std::string& textureRelativePath
)
{
    aiMaterial* mat = new aiMaterial();

    // Nome materiale
    aiString name(materialName);
    mat->AddProperty(&name, AI_MATKEY_NAME);

    // Texture (priorità a override texture)
    std::string texturePath;
    if (!textureRelativePath.empty()) {
        texturePath = textureRelativePath;
    } else if (obj->hasOverrideTexture()) {
        texturePath = obj->getOverrideTextureSourcePath();
    } else if (obj->getModel() && obj->getModel()->hasTexture()) {
        texturePath = obj->getModel()->getTextureSourcePath();
    }

    if (!texturePath.empty()) {
        aiString texPath(texturePath);
        mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
    }

    // Colore (se non c'è texture)
    if (obj->hasOverrideColor()) {
        glm::vec4 color = obj->getOverrideColor();
        aiColor4D diffuse(color.r, color.g, color.b, color.a);
        mat->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
        mat->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_AMBIENT);
    } else {
        aiColor4D defaultColor(0.8f, 0.8f, 0.8f, 1.0f);
        mat->AddProperty(&defaultColor, 1, AI_MATKEY_COLOR_DIFFUSE);
    }

    float shininess = 32.0f;
    mat->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);

    aiColor3D specular(0.5f, 0.5f, 0.5f);
    mat->AddProperty(&specular, 1, AI_MATKEY_COLOR_SPECULAR);

    return mat;
}

aiMatrix4x4 AssimpSceneExporter::glmToAiMatrix(const glm::mat4& mat) {
    // glm è column-major, Assimp row-major
    return aiMatrix4x4(
        mat[0][0], mat[1][0], mat[2][0], mat[3][0],
        mat[0][1], mat[1][1], mat[2][1], mat[3][1],
        mat[0][2], mat[1][2], mat[2][2], mat[3][2],
        mat[0][3], mat[1][3], mat[2][3], mat[3][3]
    );
}

bool AssimpSceneExporter::copyTexturesWithTracking(
    const std::unordered_map<unsigned int, std::string>& textureMap,
    const fs::path& exportDir,
    std::string& error
) {
    try {
        if (textureMap.empty()) return true;

        fs::path texturesDir = exportDir / "textures";
        fs::create_directories(texturesDir);

        // Evita copie duplicate
        std::unordered_set<std::string> copied;

        for (const auto& kv : textureMap) {
            const fs::path srcPath(kv.second);
            if (!fs::exists(srcPath)) {
                std::cerr << "[TEXTURE] File non trovato: " << srcPath << std::endl;
                continue;
            }

            const std::string filename = srcPath.filename().string();
            if (copied.insert(filename).second) {
                fs::path dstPath = texturesDir / filename;
                fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing);
                std::cout << "[TEXTURE] Copiata: " << filename << std::endl;
            }
        }

        return true;
    } catch (const std::exception& e) {
        error = std::string("Errore copia texture: ") + e.what();
        return false;
    }
}

// Embedding delle texture nella aiScene
void AssimpSceneExporter::embedTexturesIntoScene(
    aiScene* scene,
    const std::unordered_map<unsigned int, std::string>& textureMap,
    ExportResult& result
) {
    if (!scene || textureMap.empty()) return;

    // Raccogli texture valide da embed e mappa materiale->indice embedded
    std::vector<std::pair<unsigned int, std::string>> toEmbed; // (materialIndex, path)
    for (const auto& kv : textureMap) {
        const unsigned int matIdx = kv.first;
        const std::string& path   = kv.second;
        if (path.empty() || !fs::exists(path)) continue;
        toEmbed.emplace_back(matIdx, path);
    }

    if (toEmbed.empty()) return;

    // Costruisci aiTexture array
    scene->mNumTextures = static_cast<unsigned int>(toEmbed.size());
    scene->mTextures = new aiTexture*[scene->mNumTextures];

    for (size_t i = 0; i < toEmbed.size(); ++i) {
        const auto& [matIdx, path] = toEmbed[i];

        // Leggi file in memoria
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            result.warnings.push_back(std::string("Impossibile leggere la texture: ") + path);
            continue;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        if (size <= 0) {
            result.warnings.push_back(std::string("Texture vuota o illeggibile: ") + path);
            continue;
        }

        std::vector<char> buffer(static_cast<size_t>(size));
        if (!file.read(buffer.data(), size)) {
            result.warnings.push_back(std::string("Lettura fallita per texture: ") + path);
            continue;
        }

        // Crea aiTexture compressa (mHeight = 0, mWidth = size bytes)
        aiTexture* tex = new aiTexture();
        tex->mHeight = 0;
        tex->mWidth  = static_cast<unsigned int>(size);
        tex->pcData  = new aiTexel[tex->mWidth];
        std::memcpy(tex->pcData, buffer.data(), static_cast<size_t>(size));

        // Format hint (es. "png", "jpg")
        std::string ext = fs::path(path).extension().string();
        if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);
        std::string lower;
        lower.resize(ext.size());
        std::transform(ext.begin(), ext.end(), lower.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        std::snprintf(tex->achFormatHint, AI_MAX_NUMBER_OF_TEXTURECOORDS, "%s", lower.c_str());

        scene->mTextures[i] = tex;

        // Ritocca materiale per puntare a "*i"
        if (matIdx < scene->mNumMaterials && scene->mMaterials[matIdx]) {
            std::string marker = std::string("*") + std::to_string(i);
            aiString embPath(marker.c_str());
            scene->mMaterials[matIdx]->AddProperty(&embPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
        }
    }

    result.texturesEmbedded = true;
    result.texturesEmbeddedCount = scene->mNumTextures;

    // Conta mancanti (path non esistente)
    for (const auto& kv : textureMap) {
        const std::string& p = kv.second;
        if (p.empty() || !fs::exists(p)) {
            result.texturesMissing++;
        }
    }
    if (result.texturesMissing > 0) {
        result.warnings.push_back("Alcune texture non sono state incorporate perché il file di origine non è disponibile.");
    }
}

void AssimpSceneExporter::calculateSceneStats(const aiScene* scene, ExportResult& result) {
    result.totalMeshes = scene->mNumMeshes;
    result.totalMaterials = scene->mNumMaterials;
    
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        if (!mesh) continue;
        result.totalVertices += mesh->mNumVertices;
        result.totalFaces += mesh->mNumFaces;
    }
    
    std::set<std::string> uniqueTextures;
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        if (!mat) continue;
        aiString texPath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            uniqueTextures.insert(texPath.C_Str());
        }
    }
    result.totalTextures = static_cast<unsigned int>(uniqueTextures.size());
}