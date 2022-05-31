#include "AnimationBlend.h"
#include "Animation.h"
#include "SkinModel.h"
#include "Configuration.h"

CAnimationBlend::CAnimationBlend(int factor1_divides, int factor2_divides, 
	float max1, float max2, vector<HierarchyMesh*>* _meshList)
{
	Init(factor1_divides, factor2_divides, max1, max2, _meshList, NULL);
	this->_meshList = _meshList;
}

CAnimationBlend::CAnimationBlend(vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList)
{
	memset(this, 0, sizeof(CAnimationBlend)); 
	this->_meshList = _meshList;
	this->_nodeList = _nodeList;
}

void CAnimationBlend::Init(int factor1_divides, int factor2_divides, float max_f1, float max_f2,
	vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList)
{
	this->factor1_divides = factor1_divides;
	this->factor2_divides = factor2_divides;
	this->max_factor1 = max_f1;
	this->max_factor2 = max_f2;
	this->_meshList = _meshList;
	this->_nodeList = _nodeList;
}

void CAnimationBlend::SetAnimations(Animation* animation, int factor1_pos, int factor2_pos)
{
	animation_map.insert(make_pair(make_pair(factor1_pos, factor2_pos), animation));

	float factor1 = ((float) factor1_pos) / factor1_divides * max_factor1;
	float factor2 = ((float)factor2_pos) / factor2_divides * max_factor2;
	factor1_map.try_emplace(factor1, factor1_pos);
	factor2_map.try_emplace(factor2, factor2_pos);
}

const array<XMMATRIX, 96>& CAnimationBlend::GetBlendedBoneMat(float time_elapsed, float factor1, float factor2)
{
	//find key, factor
	animation_time += time_elapsed * (*animation_map.begin()).second->GetTickPerSecond();

	float ufactor;
	int u1, u;
	FindFactorKey(factor1, factor1_map, u1, u, ufactor);

	float vfactor;
	int v1, v;
	FindFactorKey(factor2, factor2_map, v1, v, vfactor);

	GetBoneMat((*animation_map.find(make_pair(u, v))).second, tmpone);
	GetBoneMat((*animation_map.find(make_pair(u, v1))).second, tmptwo);
	BlendAnimation(tmpone, tmptwo, vfactor, blend_one);
	
	GetBoneMat((*animation_map.find(make_pair(u1, v))).second, tmpone);
	GetBoneMat((*animation_map.find(make_pair(u1, v1))).second, tmptwo);
	BlendAnimation(tmpone, tmptwo, vfactor, blend_two);

	BlendAnimation(blend_one, blend_two, ufactor, result);

	return result;
}

void CAnimationBlend::GetBoneMat(Animation* anim, array<XMMATRIX, 96>& bone_list)
{
	//메쉬 정보가 없으면 리턴
	if (_meshList->empty()) 	return;

	anim->UpdateAnimation(animation_time, 0.f);

	for (auto& nodeItem : *_nodeList) {
		nodeItem->worldTM = nodeItem->localTM;
		
		anim->GetAniTM(nodeItem->name, nodeItem->worldTM);

		if (nodeItem->parent)
			nodeItem->worldTM = nodeItem->worldTM * nodeItem->parent->worldTM;
	}

	int index = 0;
	//for (const auto& mesh : *_meshList) {
		const auto& mesh = (*_meshList)[0];
		//본 정보 셋팅
		if (!mesh->boneList.empty()) {
			for (const auto& boneInfo : mesh->boneList)
				bone_list[index++] = XMMatrixTranspose(boneInfo.matOffset * boneInfo.linkNode->worldTM);
		}
	//}
	return;
}

void CAnimationBlend::BlendAnimation(const array<XMMATRIX, 96>& a, const array<XMMATRIX, 96>& b, float factor, array<XMMATRIX, 96>& result)
{
	for (int i = 0; i < a.size(); ++i) {
		for (int j = 0; j < 4; ++j) {
			result[i].r[j] = XMVectorLerp(a[i].r[j], b[i].r[j], factor);
		}
		//result[i] = a[i] * factor + b[i] * (1 - factor);
	}
}

void CAnimationBlend::CalcFactor(float factor_divides, float max_factor, float factor, int& index, float& result_factor) {
	float factor_size = 1 / factor_divides;
	float factor_factor = factor / max_factor;
	float factor_index = fmod(factor_factor, factor_size);
	index = (int)factor_index;
	result_factor = factor_factor - index;
};

//----------------------------------------------------------------------------------------------------------------------------------
//PlayerBlend

#define IDLE "idle"
#define FORWARD "forward"
#define BACKWARD "back"
#define LEFT	"left"
#define RIGHT	"right"
CPlayerBlend::CPlayerBlend(vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList)
{
	Init(_meshList, _nodeList, NULL);
}

void CPlayerBlend::Init(vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList, SkinModel* model)
{
	CAnimationBlend::Init(6, 4, 700.0f, 360.f, _meshList, _nodeList);
	this->model = model;
	auto anims = CConfiguration::player_anims;
	
	file_name_map.emplace(IDLE, anims[0]);
	file_name_map.emplace(FORWARD, anims[3]);
	file_name_map.emplace(BACKWARD, anims[1]);
	file_name_map.emplace(LEFT, anims[2]);
	file_name_map.emplace(RIGHT, anims[2]);
	auto& a =model->GetAnimationList();
	Animation* idle = &a[5];
	idle->SetDuration(30);
	//auto idle = ani_pool.GetAnimation((*file_name_map.find(IDLE)).second);

	SetAnimations(idle, 0, 0);
	SetAnimations(idle, 0, 1);
	SetAnimations(idle, 0, 2);
	SetAnimations(idle, 0, 3);
	SetAnimations(idle, 0, 4);

	/*auto forward = ani_pool.GetAnimation((*file_name_map.find(FORWARD)).second);
	auto left = ani_pool.GetAnimation((*file_name_map.find(LEFT)).second);
	auto right = ani_pool.GetAnimation((*file_name_map.find(RIGHT)).second);
	auto back = ani_pool.GetAnimation((*file_name_map.find(BACKWARD)).second);*/
	auto forward = &a[0];
	auto left = &a[1];
	auto right = &a[2];
	auto back = &a[3];

	SetAnimations(forward, 6, 0);
	SetAnimations(right, 6, 1);
	SetAnimations(back, 6, 2);
	SetAnimations(left, 6, 3);
	SetAnimations(forward, 6, 4);
	/*
	SetAnimations(forward, 0, 0);
	SetAnimations(right, 0, 1);
	SetAnimations(back, 0, 2);
	SetAnimations(left, 0, 3);
	SetAnimations(forward, 0, 4);*/
}


//----------------------------------------------------------------------------------------------
void FindFactorKey(float factor, const map<float, int>& map_, int& upperindex, int& lowerindex, float& result_factor) {
	auto found = map_.lower_bound(factor);
	if (found == map_.end()) found--;
	float ufactor = (*found).first;
	upperindex = (*found).second;

	if (found != map_.begin()) --found;
	float lfactor = (*found).first;
	lowerindex = (*found).second;

	result_factor = (factor - lfactor) / (ufactor - lfactor + 0.001f) ;
	result_factor = clamp(result_factor, 0.0f, 1.f);
};