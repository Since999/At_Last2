#include "stdafx.h"
#include "ModelLoader.h"
#include "AnimationBlend.h"
//#include "Animation.h"

//assimp�� �̿��� �� ������ �о�´�.
SkinModel* ModelLoader::LoadModel(CString path, SkinModel* model, UINT flag)
{
	//Read Model
	Assimp::Importer	importer;
	string				strPath	= CT2CA(path.operator LPCWSTR());
	const aiScene*		pScene	= importer.ReadFile(strPath, flag);
	
	if (!pScene) return NULL;

	//������ ���� ����
	Vertex					vertices;
	vector<unsigned long>	indices;
	if(model == NULL) model = new SkinModel();

	//�� �̸� ����
	wstring modelName = getFileName(path);
	model->SetName(modelName);

	//�޽� ���� ����
	for (UINT i = 0; i < pScene->mNumMeshes; i++)
		ProcessMesh(pScene->mMeshes[i], vertices, indices, model->GetMeshList());

	//���� ���� ����
	ProcessMaterial( pScene, model->GetMaterialList(), GetDirectoryPath(path));
	
	//��������(��) ���� ����
	ProcessNode(pScene->mRootNode, model);
	//������ tm ������Ʈ�� ���� ���� ���� ���� ����
	sort(model->GetNodeList().begin(), model->GetNodeList().end(),
		[](const NodeInfo* a, const NodeInfo* b)->bool {	return a->depth < b->depth; });


	//��Ű�� ����
	for (UINT i = 0; i < pScene->mNumMeshes; i++) {
		aiMesh* aiMesh = pScene->mMeshes[i];

		if (!aiMesh->HasBones()) 
			continue;

		HierarchyMesh* mesh = (HierarchyMesh*)model->GetMeshList()[i];
		ProcessSkin(aiMesh, mesh, vertices, indices, model);
	}

	//�ִϸ��̼� ���� ����
	if (pScene->HasAnimations())
		ProcessAnimation(pScene, model);
	
	//���� �� �ε��� ���� ���� ���н� ������ �� ��ü ���� �� ��ȯ
	if (!model->CreateModel(DEVICEMANAGER.pd3dDevice, DEVICEMANAGER.pd3dCommandList, vertices, indices)) {
		SAFE_RELEASE(model);
		return NULL;
	}

	//���׸��� ������ �޽� ���� ������Ʈ
	model->CompleteLoad();

	return model;
}

void ModelLoader::LoadAnimation(CString path, SkinModel* model, UINT flag)
{

	//Read Model
	Assimp::Importer	importer;
	string				strPath = CT2CA(path.operator LPCWSTR());
	const aiScene*		pScene = importer.ReadFile(strPath, flag);

	if (!pScene) return;

	//�ִϸ��̼� ���� ����
	if (pScene->HasAnimations())
		ProcessAnimation(pScene, model);
}



void ModelLoader::ProcessNode(aiNode * aiNodeInfo, SkinModel* skModel, NodeInfo* parent, int depth)
{
	//�������� ���� ���� �� �߰�
	wstring nodeName = ConvertToWString((CString)aiNodeInfo->mName.C_Str());
	XMMATRIX tm = XMMatrixTranspose(XMMATRIX(aiNodeInfo->mTransformation[0]));
	NodeInfo* node = new NodeInfo(parent, nodeName, tm, depth);
	skModel->GetNodeList().emplace_back(node);
	
	//�޽��� ����� ���̸� ��������
	if (aiNodeInfo->mNumMeshes > 0) {
		auto a = skModel->GetMeshList();
		HierarchyMesh* hiMesh = (HierarchyMesh*)a[aiNodeInfo->mMeshes[0]];
		hiMesh->linkNode = node;
	}

	//���� ��� Ž��
	for (UINT i = 0; i < aiNodeInfo->mNumChildren; i++) {
		ProcessNode(aiNodeInfo->mChildren[i], skModel, node, depth+1);
	}


}

void ModelLoader::ProcessMesh(aiMesh * mesh, Vertex& vertices, vector<unsigned long>& indices, vector<HierarchyMesh*>& meshList)
{
	UINT startIdx	= (UINT)indices.size();
	UINT startVert	= (UINT)vertices.position.size();

	XMFLOAT3 position, normal, bitangent, tangent;
	//XMFLOAT4 color;
	
	//���� ���� ����
	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		//Position
		if (!mesh->mVertices)	return;
		memcpy_s(&position, sizeof(position), &mesh->mVertices[i], sizeof(mesh->mVertices[i]));
		vertices.position.emplace_back(position);
		
		//���������� ���� ��.
		/*if (mesh->mColors) {	
			memcpy_s(&color, sizeof(color), &mesh->mColors[i], sizeof(mesh->mColors[i]));
			vertices.color.emplace_back(color);
		}*/

		//Normal
		if (mesh->mNormals) {
			memcpy_s(&normal, sizeof(normal), &mesh->mNormals[i], sizeof(mesh->mNormals[i]));
			vertices.normal.emplace_back(normal);
		}

		//Bitangent
		if (mesh->mBitangents) {
			memcpy_s(&bitangent, sizeof(bitangent), &mesh->mBitangents[i], sizeof(mesh->mBitangents[i]));
			vertices.bitangent.emplace_back(bitangent);
		}

		//Tangent
		if (mesh->mTangents) {
			memcpy_s(&tangent, sizeof(tangent), &mesh->mTangents[i], sizeof(mesh->mTangents[i]));
			vertices.tangent.emplace_back(tangent);
		}

		//UV
		if (mesh->mTextureCoords[0]) {
			vertices.uv.emplace_back(XMFLOAT2((float)mesh->mTextureCoords[0][i].x, (float)mesh->mTextureCoords[0][i].y));
		}
	}

	//�ε��� ���� ����
	for (UINT i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++) {
			indices.emplace_back(face.mIndices[j]);
			indices.back() += startVert;
		}
	}

	//�޽� ����
	
	wstring meshName = (wstring)((CString)mesh->mName.C_Str()).TrimLeft();
	meshList.emplace_back(new HierarchyMesh(meshName, startIdx, (int)indices.size() - startIdx, mesh->mMaterialIndex, startVert));
	
}

void ModelLoader::ProcessMaterial(const aiScene * pScene, vector<wstring>& matList, CString directoryPath)
{
	for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
		wstring newMat;

		// extract material info if material exists.
		if (pScene->mMaterials[i] != NULL) {

			// variables for materials
			/*aiColor3D color(0.0f, 0.0f, 0.0f);
			aiColor3D ambient(0.0f, 0.0f, 0.0f);
			aiColor3D specular(0.0f, 0.0f, 0.0f);
			aiColor3D transparent(0.0f, 0.0f, 0.0f);

			float opacity = 0.0f;
			float shininess = 0.0f;*/


			aiString  texture_path[3];
			pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path[0], NULL, NULL, NULL, NULL, NULL);
			wstring texPath = directoryPath + getFileName((CString)texture_path[0].C_Str());
			newMat = texPath;
			//newMat.diffuseMap = RM_TEXTURE.AddResource(texPath);
			/*
			pScene->mMaterials[i]->GetTexture(aiTextureType_OPACITY, 0, &texture_path[1], NULL, NULL, NULL, NULL, NULL);
			texPath = directoryPath + getFileName((CString)texture_path[1].C_Str());
			//newMat.alphaMap = RM_TEXTURE.AddResource(texPath);

			pScene->mMaterials[i]->GetTexture(aiTextureType_HEIGHT, 0, &texture_path[2], NULL, NULL, NULL, NULL, NULL);
			texPath = directoryPath + getFileName((CString)texture_path[2].C_Str());
			//newMat.normalMap = RM_TEXTURE.AddResource(texPath);
			*/
		}
		matList.emplace_back(newMat);
	}
}

void ModelLoader::ProcessSkin(aiMesh * aiMesh, HierarchyMesh * mesh, Vertex& vertices, vector<unsigned long>& indices, SkinModel* skModel)
{
	auto  node			= skModel->GetNodeList();
	auto& listBoneId	= vertices.boneidx;
	auto& listWeight	= vertices.weight;

	//���� ������ ���� ���̾ƿ����� �ѱ�� ���� ����ġ ������ �� �ε��� ������ ������ ��ġ ������ ��ġ��Ų��.
	if (listBoneId.size() < vertices.position.size()) {
		listBoneId.resize(vertices.position.size(), XMUINT4(0,0,0,0));
		listWeight.resize(vertices.position.size(), XMFLOAT4(0.f, 0.f, 0.f, 0.f));
	}
	
	//��Ű�� ���� ����
	for (UINT i = 0; i < aiMesh->mNumBones; i++) {
		auto aibone = aiMesh->mBones[i];
		
		//������ 
		BoneInfo bone;
		bone.matOffset = XMMatrixTranspose(XMMATRIX(aibone->mOffsetMatrix[0]));
		
		//����� ��� ã��
		bone.linkNode = FindNode(aibone->mName, node);
		mesh->boneList.emplace_back(bone);
	

		//����ġ ����
		for (UINT j = 0; j < aibone->mNumWeights; j++) {
			auto vertId = aibone->mWeights[j].mVertexId + mesh->startVert;
			auto weight = aibone->mWeights[j].mWeight;

			//����ġ ���� ���� ��ġ�� �Է�
			if (listWeight[vertId].x == 0) {
				listBoneId[vertId].x = i;
				listWeight[vertId].x = weight;
			}
			else if (listWeight[vertId].y == 0) {
				listBoneId[vertId].y = i;
				listWeight[vertId].y = weight;
			}
			else if (listWeight[vertId].z == 0) {
				listBoneId[vertId].z = i;
				listWeight[vertId].z = weight;
			}
			else if (listWeight[vertId].w == 0) {
				listBoneId[vertId].w = i;
				listWeight[vertId].w = weight;
			}
		}
	}
}

void ModelLoader::ProcessAnimation(const aiScene * pScene, SkinModel* skModel)
{

	//�ִϸ��̼��� ���� ��ŭ...
	for (UINT i = 0; i < pScene->mNumAnimations; i++) {
		auto aiAni = pScene->mAnimations[i];
		
		float lastTime = 0.f;
		Animation aniInfo;
		aniInfo.SetDuration((float)aiAni->mDuration);
		aniInfo.SetTickPerSecond((float)aiAni->mTicksPerSecond);

		CString aniName = (CString)aiAni->mName.C_Str();
		aniName = aniName.TrimLeft();

		if(aniName != L"")
			aniInfo.SetName((wstring)aniName);

		//����� ��� ��ŭ...
		for (UINT j = 0; j < aiAni->mNumChannels; j++) {
			auto aiAniNode = aiAni->mChannels[j];

			AniNode aniNodeInfo;
			aniNodeInfo.name = ConvertToWString((CString)aiAniNode->mNodeName.C_Str());
			UINT keyCnt = max(aiAniNode->mNumPositionKeys, aiAniNode->mNumRotationKeys);
			keyCnt		= max(keyCnt, aiAniNode->mNumScalingKeys);


			//Ű ������ ������ �����Ѵ�.
			XMFLOAT3 translation	= XMFLOAT3(0.f, 0.f, 0.f);
			XMFLOAT3 scale			= XMFLOAT3(0.f,0.f,0.f);
			XMFLOAT4 rotation		= XMFLOAT4(0.f,0.f,0.f,0.f);
			float	 time			= 0.f;

			//Ű ���� ���� ���� ������, �������� �ִ�.
			//���� ��� ���� ������ ä���.
			for (UINT k = 0; k < keyCnt; k++) {
				if (aiAniNode->mNumPositionKeys > k) {
					auto posKey = aiAniNode->mPositionKeys[k];
					memcpy_s(&translation, sizeof(translation), &posKey.mValue, sizeof(posKey.mValue));
					time = (float)aiAniNode->mPositionKeys[k].mTime;
				}

				if (aiAniNode->mNumRotationKeys > k) {
					auto rotKey = aiAniNode->mRotationKeys[k];
					rotation = XMFLOAT4(rotKey.mValue.x, rotKey.mValue.y, rotKey.mValue.z, rotKey.mValue.w);
					time = (float)aiAniNode->mRotationKeys[k].mTime;
				}

				if (aiAniNode->mNumScalingKeys > k) {
					auto scaleKey = aiAniNode->mScalingKeys[k];
					memcpy_s(&scale, sizeof(scale), &scaleKey.mValue, sizeof(scaleKey.mValue));
					time = (float)aiAniNode->mScalingKeys[k].mTime;
				}

				aniNodeInfo.keyFrame.emplace_back(KeyFrame{ time, translation, rotation, scale });
			}

			lastTime = max(aniNodeInfo.keyFrame.back().timePos, lastTime);
			aniInfo.GetAniNodeList().emplace_back(aniNodeInfo);
		}
		std::sort(aniInfo.GetAniNodeList().begin(), aniInfo.GetAniNodeList().end(), [](const AniNode& a, const AniNode& b) {
			return a.name < b.name;
		});
		aniInfo.SetLastFrame(lastTime);
		skModel->GetAnimationList().emplace_back(aniInfo);
	}
}


//�̸��� ��ġ�ϴ� ��带 ã�´�.
NodeInfo * ModelLoader::FindNode(aiString name, vector<NodeInfo*> nodeList)
{
	NodeInfo* findNode = NULL;
	wstring bName = ConvertToWString((CString)name.C_Str());

	//��� Ž��
	auto nodeInfo = find_if(nodeList.begin(), nodeList.end(), [bName](const NodeInfo* a)->bool { return a->name == bName; });

	//ã���� ��� ���� ��ȯ
	if (nodeInfo != nodeList.end())
		findNode = *nodeInfo;

	return findNode;
}

CString getFileName(CString path) {
	int index = path.ReverseFind('\\') + 1;

	if (index == 0) {
		index = path.ReverseFind('/') + 1;
	}

	CString name = path.Right(path.GetLength() - index);

	return name;
}

CString GetDirectoryPath(CString path)
{
	int index = path.ReverseFind('\\') + 1;

	if (index == 0) {
		index = path.ReverseFind('/') + 1;
	}

	CString dirPath = path.Left(index);

	return dirPath;
}

wstring ConvertToWString(CString& str)
{
	return (wstring)str;
}



//-----------------------------------------------------------------------------------------------------------------
//Load Animation Only

Animation ModelLoader::ProcessAnimation(const aiScene* pScene)
{

	//�ִϸ��̼��� ���� ��ŭ...
	for (UINT i = 0; i < pScene->mNumAnimations; i++) {
		auto aiAni = pScene->mAnimations[i];

		float lastTime = 0.f;
		Animation aniInfo;
		aniInfo.SetDuration((float)aiAni->mDuration);
		aniInfo.SetTickPerSecond((float)aiAni->mTicksPerSecond);

		CString aniName = (CString)aiAni->mName.C_Str();
		aniName = aniName.TrimLeft();

		if (aniName != L"")
			aniInfo.SetName((wstring)aniName);

		//����� ��� ��ŭ...
		for (UINT j = 0; j < aiAni->mNumChannels; j++) {
			auto aiAniNode = aiAni->mChannels[j];

			AniNode aniNodeInfo;
			aniNodeInfo.name = ConvertToWString((CString)aiAniNode->mNodeName.C_Str());
			UINT keyCnt = max(aiAniNode->mNumPositionKeys, aiAniNode->mNumRotationKeys);
			keyCnt = max(keyCnt, aiAniNode->mNumScalingKeys);


			//Ű ������ ������ �����Ѵ�.
			XMFLOAT3 translation = XMFLOAT3(0.f, 0.f, 0.f);
			XMFLOAT3 scale = XMFLOAT3(0.f, 0.f, 0.f);
			XMFLOAT4 rotation = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
			float	 time = 0.f;

			//Ű ���� ���� ���� ������, �������� �ִ�.
			//���� ��� ���� ������ ä���.
			for (UINT k = 0; k < keyCnt; k++) {
				if (aiAniNode->mNumPositionKeys > k) {
					auto posKey = aiAniNode->mPositionKeys[k];
					memcpy_s(&translation, sizeof(translation), &posKey.mValue, sizeof(posKey.mValue));
					time = (float)aiAniNode->mPositionKeys[k].mTime;
				}

				if (aiAniNode->mNumRotationKeys > k) {
					auto rotKey = aiAniNode->mRotationKeys[k];
					rotation = XMFLOAT4(rotKey.mValue.x, rotKey.mValue.y, rotKey.mValue.z, rotKey.mValue.w);
					time = (float)aiAniNode->mRotationKeys[k].mTime;
				}

				if (aiAniNode->mNumScalingKeys > k) {
					auto scaleKey = aiAniNode->mScalingKeys[k];
					memcpy_s(&scale, sizeof(scale), &scaleKey.mValue, sizeof(scaleKey.mValue));
					time = (float)aiAniNode->mScalingKeys[k].mTime;
				}

				aniNodeInfo.keyFrame.emplace_back(KeyFrame{ time, translation, rotation, scale });
			}

			lastTime = max(aniNodeInfo.keyFrame.back().timePos, lastTime);
			aniInfo.GetAniNodeList().emplace_back(aniNodeInfo);
		}
		std::sort(aniInfo.GetAniNodeList().begin(), aniInfo.GetAniNodeList().end(), [](const AniNode& a, const AniNode& b) {
			return a.name < b.name;
		});
		aniInfo.SetLastFrame(lastTime);
		return aniInfo;
	}
}

Animation ModelLoader::LoadAnimation(const std::string& path, UINT flag)
{
	//Read Model
	Assimp::Importer	importer;
	string				strPath = path;
	const aiScene* pScene = importer.ReadFile(strPath, flag);

	if (!pScene) return Animation();

	//�ִϸ��̼� ���� ����
	if (pScene->HasAnimations())
		return ProcessAnimation(pScene);
}