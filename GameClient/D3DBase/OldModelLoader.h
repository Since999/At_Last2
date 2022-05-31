#include <map>
#include "stdafx.h"
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//#define PRINT_TEXTURE_FILE_NAMES

struct BoneInfd
{
    DirectX::XMMATRIX boneOffset;
    DirectX::XMMATRIX finalTransform;
};

class OldModelLoader {
    std::vector<std::string> texture_name;
    const aiScene* scene;
    Assimp::Importer importer;
    std::array<XMMATRIX, 96> boneTransforms;
    std::array<BoneInfd, 96> boneInfos;
    std::map<std::string, int> boneMap;
    bool with_animation = true;
    unsigned int num_textures;

    CLoadedMesh* ProcessMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        aiMesh* mesh, const aiScene* scene);

    void ProcessNode(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        aiNode* node, const aiScene* scene, std::vector<CMesh*>& meshes);

    void ProcessNodeWithTranslation(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        aiNode* node, const aiScene* scene, std::vector<CMesh*>& meshes, const XMMATRIX& parent_matrix);

    CLoadedMesh* ProcessMeshWithTranslation(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        aiMesh* mesh, const aiScene* scene, const XMMATRIX& matrix);

    // 재귀 호출을 통해 자식 노드로 내려가면서 각각 매칭된 boneTransformation을 저장하는 함수
    void ReadNodeHierarchy(
        float animationTime,
        const aiNode* node,
        const XMMATRIX& parentTransform);

    aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string nodeName);

    aiVector3D CalcInterpolatedValueFromKey(float animationTime, const unsigned int numKeys, const aiVectorKey* const vectorKey) const;

    aiQuaternion CalcInterpolatedValueFromKey(float animationTime, const unsigned int numKeys, const aiQuatKey* const quatKey) const;

    unsigned int FindKeyIndex(const float animationTime, const int numKeys, const aiVectorKey* const vectorKey) const;

    unsigned int FindKeyIndex(const float animationTime, const int numKeys, const aiQuatKey* const quatKey) const;
public:
    OldModelLoader();
    ~OldModelLoader();

    void LoadModel(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        const std::string& file, std::vector<CMesh*>& meshes);

    void LoadModelWithTranslation(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        const std::string& file, std::vector<CMesh*>& meshes);

    std::array<XMMATRIX, 96> ExtractBoneTransforms(float animationTime, const int animationIndex);

    float GetAnimationTime(const unsigned int animationIndex) const {
        if (!with_animation) return 0.0f;
        assert(scene->mNumAnimations > animationIndex);
        return (float)scene->mAnimations[animationIndex]->mDuration;
    }

    const unsigned int GetNumTexture() { return num_textures; }

    void printall(const std::string& file);
};

XMMATRIX ConvertToMatrix(const aiMatrix4x4 mat);

XMFLOAT3 ConvertToXMFLOAT3(const aiVector3D& vec);
