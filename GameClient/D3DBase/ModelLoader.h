//==================================================================
//		## ModelLoader ## (assimp 라이브러리를 이용해 모델 정보를 로드한다.)
//==================================================================

#pragma once
#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include <atlstr.h>

#include "SkinModel.h"
#include "ModelStructure.h"

class ModelLoader
{
public:
	ModelLoader() {};
	~ModelLoader() {};

	//모델 정보 로드
	static SkinModel*	LoadModel(CString path, SkinModel* model = NULL, 
		UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);
	static void		LoadAnimation(CString path, SkinModel* model, UINT flag);
	static Animation LoadAnimation(const std::string& path, UINT flag = aiProcess_MakeLeftHanded );
protected:
	//노드 정보 생성(본 정보)
	static void ProcessNode(aiNode* aiNodeInfo, SkinModel* skModel, NodeInfo* parent = NULL, int depth = 0);
	//메쉬 정보 생성(정점정보, 인덱스 정보, 기본 메쉬 정보)
	static void ProcessMesh(aiMesh* mesh, Vertex& vertices, vector<unsigned long>& indices, vector<HierarchyMesh*>& meshList);
	//재질 정보 생성
	static void ProcessMaterial(const aiScene* pScene, vector<wstring>& matList, CString directoryPath);
	//스키닝 정보 생성(offset_mat, weight)
	static void ProcessSkin(aiMesh* aiMesh, HierarchyMesh* mesh, Vertex& vertices, vector<unsigned long>& indices, SkinModel* skModel);
	//애니메이션 정보 생성
	static void ProcessAnimation(const aiScene* pScene, SkinModel* skModel);

	static Animation ProcessAnimation(const aiScene* pScene);

	//이름을 통해 노드를 찾는다.
	static NodeInfo* FindNode(aiString name, vector<NodeInfo*> nodeList);
};

CString getFileName(CString path);

CString GetDirectoryPath(CString path);

wstring ConvertToWString(CString& str);