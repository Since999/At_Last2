#include "OldModelLoader.h"

OldModelLoader::OldModelLoader()
{
    ZeroMemory(&boneTransforms, sizeof(boneTransforms));
}
OldModelLoader::~OldModelLoader()
{
}


void OldModelLoader::LoadModel(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    const std::string& file, std::vector<CMesh*>& meshes)
{
    scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);

    if (!scene) {
        std::string a(importer.GetErrorString());
        return;
    }

    if (scene->mNumAnimations == 0) with_animation = false;

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        // error message
        std::string a(importer.GetErrorString());
        return;
    }
    //directory = path.substr(0, path.find_last_of('/'));

    ProcessNode(pd3dDevice, pd3dCommandList, scene->mRootNode, scene, meshes);
}

void OldModelLoader::ProcessNode(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    aiNode* node, const aiScene* scene, std::vector<CMesh*>& meshes)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {

        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(pd3dDevice, pd3dCommandList, mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(pd3dDevice, pd3dCommandList, node->mChildren[i], scene, meshes);
    }
}

CLoadedMesh* OldModelLoader::ProcessMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    aiMesh* mesh, const aiScene* scene)
{
    std::vector<CAnimationVertex> vertices;
    //std::vector<unsigned int> indices;
    //std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        CAnimationVertex vertex;
        // vertex position, normal, uv
        vertex.m_xmf3Position.x = mesh->mVertices[i].x;
        vertex.m_xmf3Position.y = mesh->mVertices[i].y;
        vertex.m_xmf3Position.z = mesh->mVertices[i].z + 500;
        if (mesh->mTextureCoords[0]) {
            vertex.m_xmf2TexCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.m_xmf2TexCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.m_xmf2TexCoord.x = 0;
            vertex.m_xmf2TexCoord.y = 0;
        }
        if (mesh->HasNormals()) vertex.Normal = ConvertToXMFLOAT3(mesh->mNormals[i]);

        vertices.push_back(vertex);
    }
    //bone
    int numBones = 0;

    std::vector<CAnimationVertex> real_vertices;
    // indices 
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            real_vertices.push_back(vertices[face.mIndices[j]]);
        }
    }
    // material 
    if (mesh->mMaterialIndex >= 0)
    {

    }
    return new CLoadedMesh(pd3dDevice, pd3dCommandList, real_vertices);
}

//-----------------------------------------------------------------------------------------------------------------

void OldModelLoader::LoadModelWithTranslation(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    const std::string& file, std::vector<CMesh*>& meshes)
{
    scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);

    if (scene->mNumAnimations == 0) with_animation = false;
    //const aiScene* scene = importer.ReadFile(file, NULL);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        //error message
        std::string a(importer.GetErrorString());
        return;
    }

    ProcessNodeWithTranslation(pd3dDevice, pd3dCommandList, scene->mRootNode, scene, meshes, XMMatrixIdentity());

#ifdef PRINT_TEXTURE_FILE_NAMES
    printall(file);
#endif
    return;
}

void OldModelLoader::ProcessNodeWithTranslation(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    aiNode* node, const aiScene* scene, std::vector<CMesh*>& meshes, const XMMATRIX& parent_matrix)
{
    //XMMATRIX node_transform = XMMatrixIdentity();
    XMMATRIX node_transform = XMMatrixTranspose(ConvertToMatrix(node->mTransformation)) * parent_matrix;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMeshWithTranslation(pd3dDevice, pd3dCommandList, mesh, scene, node_transform));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNodeWithTranslation(pd3dDevice, pd3dCommandList, node->mChildren[i], scene, meshes, node_transform);
    }
}

CLoadedMesh* OldModelLoader::ProcessMeshWithTranslation(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    aiMesh* mesh, const aiScene* scene, const XMMATRIX& matrix)
{
    std::vector<CAnimationVertex> vertices;
    //std::vector<unsigned int> indices;
    //std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        CAnimationVertex vertex;
        // vertex position, normal, uv
        XMVECTOR ver{ mesh->mVertices[i].x , mesh->mVertices[i].y,mesh->mVertices[i].z, 1.0f };
        ver = XMVector4Transform(ver, matrix);
        vertex.m_xmf3Position.x = ver.m128_f32[0];
        vertex.m_xmf3Position.y = ver.m128_f32[1];
        vertex.m_xmf3Position.z = ver.m128_f32[2] + 500;
        if (mesh->mTextureCoords[0]) {
            vertex.m_xmf2TexCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.m_xmf2TexCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.m_xmf2TexCoord.x = 0;
            vertex.m_xmf2TexCoord.y = 0;
        }
        if (mesh->HasNormals()) {
            XMVECTOR normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 1.0f };
            normal = XMVector4Transform(normal, matrix);
            XMStoreFloat3(&vertex.Normal, normal);
            vertex.Normal = Vector3::Normalize(vertex.Normal);
        }

        vertices.push_back(vertex);
    }
    //bone
    int numBones = 0;
    for (unsigned int i = 0; i < mesh->mNumBones; i++)
    {
        unsigned int boneIndex = numBones++;
        boneMap.insert(std::make_pair(std::string(mesh->mBones[boneIndex]->mName.data), boneIndex));

        boneInfos[boneIndex].boneOffset = XMMatrixTranspose(ConvertToMatrix(mesh->mBones[boneIndex]->mOffsetMatrix));
        //boneInfos[boneIndex].boneOffset = ConvertToMatrix(mesh->mBones[boneIndex]->mOffsetMatrix);

        for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; j++)
        {
            unsigned int vertexId = mesh->mBones[i]->mWeights[j].mVertexId;
            float weight = mesh->mBones[i]->mWeights[j].mWeight;

            for (int k = 0; k < 4; k++)
            {
                if (vertices[vertexId].GetWeight(k) < 0.01f)
                {
                    vertices[vertexId].SetIndex(k, boneIndex);
                    vertices[vertexId].SetWeight(k, weight);
                    break;
                }
            }
        }
    }

    std::vector<CAnimationVertex> real_vertices;
    // indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            real_vertices.push_back(vertices[face.mIndices[j]]);
        }
    }
    // material
    aiString str;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiTextureType type = aiTextureType_DIFFUSE;

        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
        {
            material->GetTexture(type, i, &str);
            texture_name.push_back(str.C_Str());
            num_textures++;
        }
        type = aiTextureType_SPECULAR;
        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); i++)
        {
            //aiString str;
            //material->GetTexture(type, i, &str);
            //texture_name.push_back(str.C_Str());
        }
    }
    std::string tmp{ str.C_Str() };
    if (!tmp.empty()) {
        auto point = tmp.find("..\\");
        if (point != std::string::npos) {
            tmp.erase(tmp.begin() + point, tmp.begin() + point + 3);
        }
        tmp.insert(0, "Resources\\Texture\\");
    }
    std::wstring ws{ tmp.begin(), tmp.end() };
    return new CLoadedMesh(pd3dDevice, pd3dCommandList, real_vertices, ws);
}


//



std::array<XMMATRIX, 96> OldModelLoader::ExtractBoneTransforms(float animationTime, const int animationIndex)
{
    if (!scene) return boneTransforms;
    if (!with_animation) return boneTransforms;
    assert(scene->mNumAnimations > animationIndex);
    ReadNodeHierarchy(animationTime, scene->mRootNode, XMMatrixIdentity());
    //ReadNodeHierarchy(animationTime, scene->mRootNode, ConvertToMatrix(scene->mRootNode->mTransformation));

    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumBones; i++)
    {
        boneTransforms[i] = boneInfos[i].finalTransform;
        //boneTransforms[i] = boneInfos[i].boneOffset;
    }
    return boneTransforms;
}

void OldModelLoader::ReadNodeHierarchy(
    float animationTime,
    const aiNode* node,
    const XMMATRIX& parentTransform)
{
    std::string nodeName(node->mName.data);

    const aiAnimation* animation = scene->mAnimations[0];
    //XMMATRIX nodeTransform = ConvertToMatrix(node->mTransformation);
    XMMATRIX nodeTransform = XMMatrixTranspose(XMMATRIX(&node->mTransformation.a1));

    const aiNodeAnim* nodeAnim = FindNodeAnim(animation, nodeName);

    if (nodeAnim)
    {
        const aiVector3D& scaling = CalcInterpolatedValueFromKey(animationTime, nodeAnim->mNumScalingKeys, nodeAnim->mScalingKeys);
        XMMATRIX scalingM = XMMatrixScaling(scaling.x, scaling.y, scaling.z);

        const aiQuaternion& rotationQ = CalcInterpolatedValueFromKey(animationTime, nodeAnim->mNumRotationKeys, nodeAnim->mRotationKeys);

        XMMATRIX rotationM = XMMatrixRotationQuaternion(XMVectorSet(rotationQ.x, rotationQ.y, rotationQ.z, rotationQ.w));

        const aiVector3D& translation = CalcInterpolatedValueFromKey(animationTime, nodeAnim->mNumPositionKeys, nodeAnim->mPositionKeys);
        XMMATRIX translationM = XMMatrixTranslation(translation.x, translation.y, translation.z);

        nodeTransform = scalingM * rotationM * translationM;
    }

    XMMATRIX globalTransform = nodeTransform * parentTransform;

    if (boneMap.find(nodeName) != boneMap.end())
    {
        unsigned int boneIndex = boneMap[nodeName];

        boneInfos[boneIndex].finalTransform =
            boneInfos[boneIndex].boneOffset * globalTransform *
            ConvertToMatrix(scene->mRootNode->mTransformation);

        int i = 0;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ReadNodeHierarchy(animationTime, node->mChildren[i], globalTransform);
}

aiNodeAnim* OldModelLoader::FindNodeAnim(const aiAnimation* animation, const std::string nodeName)
{
    for (unsigned int i = 0; i < animation->mNumChannels; i++)
        if (animation->mChannels[i]->mNodeName.data == nodeName)
            return animation->mChannels[i];
    return nullptr;
}

aiVector3D OldModelLoader::CalcInterpolatedValueFromKey(float animationTime, const unsigned int numKeys, const aiVectorKey* const vectorKey) const
{
    aiVector3D ret;
    if (numKeys == 1)
    {
        ret = vectorKey[0].mValue;
        return ret;
    }

    unsigned int keyIndex = FindKeyIndex(animationTime, numKeys, vectorKey);
    unsigned int nextKeyIndex = keyIndex + 1;

    assert(nextKeyIndex < numKeys);

    float deltaTime = (float)(vectorKey[nextKeyIndex].mTime - vectorKey[keyIndex].mTime);
    float factor = (animationTime - (float)vectorKey[keyIndex].mTime) / deltaTime;

    assert(factor >= 0.0f && factor <= 1.0f);

    const aiVector3D& startValue = vectorKey[keyIndex].mValue;
    const aiVector3D& endValue = vectorKey[nextKeyIndex].mValue;

    ret.x = startValue.x + (endValue.x - startValue.x) * factor;
    ret.y = startValue.y + (endValue.y - startValue.y) * factor;
    ret.z = startValue.z + (endValue.z - startValue.z) * factor;

    return ret;
}

aiQuaternion OldModelLoader::CalcInterpolatedValueFromKey(float animationTime, const unsigned int numKeys, const aiQuatKey* const quatKey) const
{
    aiQuaternion ret;
    if (numKeys == 1)
    {
        ret = quatKey[0].mValue;
        return ret;
    }

    unsigned int keyIndex = FindKeyIndex(animationTime, numKeys, quatKey);
    unsigned int nextKeyIndex = keyIndex + 1;

    assert(nextKeyIndex < numKeys);

    float deltaTime = (float)(quatKey[nextKeyIndex].mTime - quatKey[keyIndex].mTime);
    float factor = (animationTime - (float)quatKey[keyIndex].mTime) / deltaTime;

    assert(factor >= 0.0f && factor <= 1.0f);

    const aiQuaternion& startValue = quatKey[keyIndex].mValue;
    const aiQuaternion& endValue = quatKey[nextKeyIndex].mValue;
    aiQuaternion::Interpolate(ret, startValue, endValue, factor);
    ret = ret.Normalize();

    return ret;
}

unsigned int OldModelLoader::FindKeyIndex(const float animationTime, const int numKeys, const aiVectorKey* const vectorKey) const
{
    assert(numKeys > 0);
    for (int i = 0; i < numKeys - 1; i++)
        if (animationTime < (float)vectorKey[i + 1].mTime)
            return i;
    assert(0);
    return 0;
}

unsigned int OldModelLoader::FindKeyIndex(const float animationTime, const int numKeys, const aiQuatKey* const quatKey) const
{
    assert(numKeys > 0);
    for (int i = 0; i < numKeys - 1; i++)
        if (animationTime < (float)quatKey[i + 1].mTime)
            return i;
    assert(0);
    return 0;
}

XMMATRIX ConvertToMatrix(const aiMatrix4x4 mat)
{
    XMFLOAT4X4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = mat[i][j];
        }
    }
    return XMLoadFloat4x4(&result);
}

#include <fstream>
void OldModelLoader::printall(const std::string& file)
{
    size_t size = file.rfind('/', file.length());
    if (size != std::wstring::npos)
    {
        std::string file_name{ file.substr(size + 1, file.length() - size) };
        file_name.append(".txt");
        std::ofstream out{ file_name };
        for (const std::string& str : texture_name) {
            out << str << std::endl;
        }
        return;
    }

    std::string file_name{ file.begin(), file.end() };
    file_name.append(".txt");
    std::ofstream out{ file_name };
    for (const std::string& str : texture_name) {
        out << str << std::endl;
    }
    return;
}


XMFLOAT3 ConvertToXMFLOAT3(const aiVector3D& vec)
{
    return XMFLOAT3{ vec.x, vec.y, vec.z };
}
