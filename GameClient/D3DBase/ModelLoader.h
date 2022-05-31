//==================================================================
//		## ModelLoader ## (assimp ���̺귯���� �̿��� �� ������ �ε��Ѵ�.)
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

	//�� ���� �ε�
	static SkinModel*	LoadModel(CString path, SkinModel* model = NULL, 
		UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);
	static void		LoadAnimation(CString path, SkinModel* model, UINT flag);
	static Animation LoadAnimation(const std::string& path, UINT flag = aiProcess_MakeLeftHanded );
protected:
	//��� ���� ����(�� ����)
	static void ProcessNode(aiNode* aiNodeInfo, SkinModel* skModel, NodeInfo* parent = NULL, int depth = 0);
	//�޽� ���� ����(��������, �ε��� ����, �⺻ �޽� ����)
	static void ProcessMesh(aiMesh* mesh, Vertex& vertices, vector<unsigned long>& indices, vector<HierarchyMesh*>& meshList);
	//���� ���� ����
	static void ProcessMaterial(const aiScene* pScene, vector<wstring>& matList, CString directoryPath);
	//��Ű�� ���� ����(offset_mat, weight)
	static void ProcessSkin(aiMesh* aiMesh, HierarchyMesh* mesh, Vertex& vertices, vector<unsigned long>& indices, SkinModel* skModel);
	//�ִϸ��̼� ���� ����
	static void ProcessAnimation(const aiScene* pScene, SkinModel* skModel);

	static Animation ProcessAnimation(const aiScene* pScene);

	//�̸��� ���� ��带 ã�´�.
	static NodeInfo* FindNode(aiString name, vector<NodeInfo*> nodeList);
};

CString getFileName(CString path);

CString GetDirectoryPath(CString path);

wstring ConvertToWString(CString& str);