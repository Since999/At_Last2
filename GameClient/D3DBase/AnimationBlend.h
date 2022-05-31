#pragma once

#include "stdafx.h"
#include "AnimationPool.h"
#include <map>
class Animation;
class SkinModel;
struct HierarchyMesh;
struct NodeInfo;

class CAnimationBlend{
private:
	std::map <std::pair<int, int>, Animation*> animation_map;
	std::map<float, int> factor1_map;
	std::map<float, int> factor2_map;
	float max_factor1;
	float max_factor2;

	int factor1_divides;
	int factor2_divides;
	float animation_time;
	
	array<XMMATRIX, 96> tmpone, tmptwo, blend_one, blend_two, result;
	vector<HierarchyMesh*>*	_meshList;
	vector<NodeInfo*>*		_nodeList;

protected:
	std::map<std::string, std::string> file_name_map;
	CAnimationPool ani_pool;
	SkinModel* model;

private:
	void GetBoneMat(Animation* anim, array<XMMATRIX, 96>& bone_list);
	
	

public:
	CAnimationBlend(int factor1_divides, int factor2_divides, float max_f1, float max_f2, vector<HierarchyMesh*>* _meshList);
	CAnimationBlend(vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList);
	CAnimationBlend() {}
	
	void Init(int factor1_divides, int factor2_divides, float max_f1, float max_f2, 
		vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList);
	void SetAnimations(Animation* animation, int factor1_pos, int factor2_pos);
	const array<XMMATRIX, 96>& GetBlendedBoneMat(float time_elapsed, float factor1, float factor2);

	static void BlendAnimation(const array<XMMATRIX, 96>& a, const array<XMMATRIX, 96>& b,
		float factor, array<XMMATRIX, 96>& result);
	static void CalcFactor(float factor_divides, float max_factor, float factor, int& index, float& result_factor);
};

class CPlayerBlend : public CAnimationBlend{

public:
	CPlayerBlend(vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList);
	CPlayerBlend() { }

	void Init(vector<HierarchyMesh*>* _meshList, vector<NodeInfo*>* _nodeList, SkinModel* model);
};

void FindFactorKey(float factor, const map<float, int>& map_, int& upperindex, int& lowerindex, float& result_factor);