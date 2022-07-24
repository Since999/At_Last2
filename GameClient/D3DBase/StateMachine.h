#pragma once
#include "stdafx.h"

class CPlayer;
class SkinModel;
class CStateMachine;

class CStateNode {
private:
public:
	CStateNode(const std::string& name, std::function<void(CStateMachine*, float)> func, std::function<bool(CStateMachine*)> change_func, CStateNode* next_node)
		: name(name), func(func), state_change_func(change_func), next_node(next_node) { };
	//void Update(float time_elapsed);
	std::string name;
	std::function<void(CStateMachine*, float)> func;
	std::function<bool(CStateMachine*)> state_change_func;
	CStateNode* next_node;
};

class CStateMachine
{
protected:
	SkinModel* model;
	CPlayer* object;
	CStateNode* curr_node = NULL;
	std::vector<CStateNode> nodes;
	bool is_anim_ended = false;
	float animation_time = 0.f;
	int						_playAniIdx = -1;
	//blend
	float max_blend_time = 0.f;
	float blend_time = 0.f;
	array<XMMATRIX, 96> blend_mat;
public:
	CStateMachine(SkinModel* model, CPlayer* object) : model(model), object(object) {}

	virtual void Update(float time_elapsed);
	virtual const array<XMMATRIX, 96>& GetBoneMat();
	void ChangeAni() { _playAniIdx += 1;
	_playAniIdx = _playAniIdx % 5;
	}
protected:
	void PlayAni(int index);
	void ChangeAniWithBlend(int index, const array<XMMATRIX, 96>& start_mat, float max_time = 0.5f);
	//Node Functions
public:
	static bool IsAnimEnd(CStateMachine*);
	static void TmpUpdate(CStateMachine*, float);
};

class CZombieStateMachine : public CStateMachine {
private:
	ZombieAnimationState anim_state;
	bool is_dead = false;
public:
	CZombieStateMachine(CPlayer* object);

	virtual void Update(float time_elapsed);
	virtual const array<XMMATRIX, 96>& GetBoneMat();
// Node Functions
public:
	static bool DoAttack(CStateMachine*);
};

class CPlayerStateMachine : public CStateMachine {
private:
	ClientAnimationState anim_state;
	float speed;
	float angle;
public:
	CPlayerStateMachine(CPlayer* object);

	virtual void Update(float time_elapsed);
	virtual const array<XMMATRIX, 96>& GetBoneMat();

	void PlayReloadAnim();
	// Node Functions
public:
	static bool IsWalking(CStateMachine*);
	static bool IsAttacking(CStateMachine*);
	
};