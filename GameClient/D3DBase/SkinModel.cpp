#include "stdafx.h"
#include "SkinModel.h"
#include "Object.h"
#include "TexturePool.h"
SkinModel::SkinModel()
{
	//_texShader	= RM_SHADER.AddResource(L"source\\shader\\texture");
	//_skinShader = RM_SHADER.AddResource(L"source\\shader\\skin");
	//SetShader(_skinShader);
}

SkinModel::~SkinModel()
{
}

bool SkinModel::CreateModel(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	Vertex& vertices, vector<unsigned long>& indices)
{
	std::vector<CAnimationVertex> real_vertices;
	for (unsigned int i : indices)
	{
		CAnimationVertex tmp;
		tmp.m_xmf3Position = vertices.position[i];
		tmp.Normal = vertices.normal[i];
		tmp.m_xmf2TexCoord = vertices.uv[i];
		tmp.boneIndex = (vertices.boneidx[i]);
		tmp.boneWeights = vertices.weight[i];
		real_vertices.push_back(tmp);
		
	}
	mesh = new CLoadedMesh(pd3dDevice, pd3dCommandList, real_vertices);
	return true;
}

void SkinModel::Release()
{
	for (auto item : _meshList)
		SAFE_DELETE(item);

	for (auto item : _nodeList)
		SAFE_DELETE(item);

	vector<NodeInfo*>().swap(_nodeList);
	vector<HierarchyMesh*>().swap(_meshList);
}

array<XMMATRIX, 96>& SkinModel::GetBoneMat()
{
	if (_meshList.empty()) 	return calcBoneList;

	int index = 0;
	const auto& mesh = _meshList[0];
	//for (const auto& mesh : _meshList) {
		//본 정보 셋팅
		if (!mesh->boneList.empty()) {
			for (const auto& boneInfo : mesh->boneList) {
				calcBoneList[index++] = XMMatrixTranspose(boneInfo.matOffset * boneInfo.linkNode->worldTM);
			}
		}
	//}
	return calcBoneList;
}

array<XMMATRIX, 96>& SkinModel::GetBoneMat(float& animation_time, float time_elapsed)
{
	//메쉬 정보가 없으면 리턴
	if (_meshList.empty()) 	return calcBoneList;

	//Node Update
	UpdateNodeTM(animation_time, time_elapsed);
	return GetBoneMat();
}

array<XMMATRIX, 96>& SkinModel::GetBoneMat(float& animation_time, float time_elapsed, Animation* anim)
{
	//메쉬 정보가 없으면 리턴
	if (_meshList.empty()) 	return calcBoneList;

	//Node Update
	UpdateNodeTM(animation_time, time_elapsed, anim);
	return GetBoneMat();
}

array<XMMATRIX, 96>& SkinModel::GetBoneMatLastFrame(int ani_index)
{
	bool result = false;

	if (ani_index == -1) ani_index = _playAniIdx;

	if (ani_index >= 0) {
		result = _aniList[ani_index].UpdateAnimationLastFrame();
	}

	for (auto& nodeItem : _nodeList) {
		nodeItem->worldTM = nodeItem->localTM;

		if (ani_index >= 0)
			_aniList[ani_index].GetAniTM(nodeItem->name, nodeItem->worldTM);

		if (nodeItem->parent)
			nodeItem->worldTM = nodeItem->worldTM * nodeItem->parent->worldTM;
	}

	return GetBoneMat();
}

void SkinModel::PlayAni(int idx)
{
	if (_playAniIdx != idx) {
		_aniList[idx].Stop();
		_playAniIdx = idx;
	}

	_aniList[idx].Play();
}

void SkinModel::SetAniRepeat(unsigned int model_index, bool is_repeat)
{
	if (_aniList.size() >= model_index) return;
	_aniList[model_index].SetRepeat(is_repeat);
}


void SkinModel::CompleteLoad()
{
	//_meshByMaterial.clear();
	//_meshByMaterial.resize(_materialList.size());


	//for (auto& mesh : _meshList) {
	//	//_meshByMaterial[mesh->matIdx].push_back(mesh);
	//}
}


bool SkinModel::UpdateNodeTM(float& time, float time_elapsed, int ani_index)
{
	bool result = false;
	if (_aniList.empty()) return true;

	if (ani_index == -1) ani_index = _playAniIdx;

	if (ani_index >= 0) {
		result = _aniList[ani_index].UpdateAnimation(time, time_elapsed);
	}

	for (auto& nodeItem : _nodeList) {
		nodeItem->worldTM = nodeItem->localTM;
		
		if (ani_index >= 0)
			_aniList[ani_index].GetAniTM(nodeItem->name, nodeItem->worldTM);

		if (nodeItem->parent)
			nodeItem->worldTM = nodeItem->worldTM * nodeItem->parent->worldTM;
	}
	return result;
}
bool SkinModel::UpdateNodeTM(float& time, float time_elapsed, Animation* anim)
{
	bool result = false;
	if (anim) {
		result = anim->UpdateAnimation(time, time_elapsed);
	}

	for (auto& nodeItem : _nodeList) {
		nodeItem->worldTM = nodeItem->localTM;

		if (anim) {
			anim->GetAniTM(nodeItem->name, nodeItem->worldTM);
		}
		if (nodeItem->parent)
			nodeItem->worldTM = nodeItem->worldTM * nodeItem->parent->worldTM;
	}
	return result;
}

void SkinModel::Render(ID3D12GraphicsCommandList* command_list)
{
	/*int i = 0;
	for (auto& mesh : meshs) {
		if (i < textures.size()) {
			textures[i]->UpdateShaderVariables(command_list);
		}
		mesh->Render(command_list);
	}*/
	if (texture) {
		texture->UpdateShaderVariables(command_list);
		//command_list->SetDescriptorHeaps(1, &descriptor_heap);	
	}
	mesh->Render(command_list);
}
void SkinModel::ShadowRender(ID3D12GraphicsCommandList* commandList)
{
	mesh->Render(commandList);
}

void SkinModel::SetTexture(CTexture* texture) { 
	this->texture = texture;
}

//--------------------------------------------------------------------------------------------------
//BlendSkinModel

CBlendSkinModel::CBlendSkinModel()
{
	
}

void CBlendSkinModel::CompleteLoad()
{
	//CBlendSkinModel(&_meshList);
	blend.Init(&_meshList, &_nodeList, this);
}

const array<XMMATRIX, 96>& CBlendSkinModel::GetBoneMat(const float& animation_time, float factor1, float factor2)
{
	return blend.GetBlendedBoneMat(animation_time, factor1, factor2);
}