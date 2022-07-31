#include "StateMachine.h"
#include "player.h"
#include "AnimationBlend.h"

void CStateMachine::Update(float time_elapsed)
{
	is_anim_ended = model->UpdateNodeTM(animation_time, time_elapsed, _playAniIdx);

	//curr_node->func(time_elapsed);
	//if (curr_node->state_change_func()) {
		//curr_node = curr_node->next_node;
	//}
}

const array<XMMATRIX, 96>& CStateMachine::GetBoneMat()
{
	Update(TIMEMANAGER.GetTimeElapsed());
	auto& mat = model->GetBoneMat();
	if (blend_time < max_blend_time) {
		CAnimationBlend::BlendAnimation(blend_mat, mat, blend_time / max_blend_time, mat);
	}
	return mat;
}

void CStateMachine::PlayAni(int idx)
{
	if (_playAniIdx != idx) {
		//_aniList[idx].Stop();
		_playAniIdx = idx;
	}
	animation_time = 0.f;
	//_aniList[idx].Play();
}

void CStateMachine::ChangeAniWithBlend(int idx, const array<XMMATRIX, 96>& start_mat, float max_time)
{
	PlayAni(idx);
	blend_mat = start_mat;
	blend_time = 0.f;
	max_blend_time = max_time;
}

#include "CAnimationObject.h"
CZombieStateMachine::CZombieStateMachine(CPlayer* object)
	:CStateMachine(object->model, object)
{
	/*std::string name{ "spawn" };
	std::function<void(CStateMachine*, float)> func;
	std::function<bool(CStateMachine*)> state_change_func{ this->IsAnimEnd };
	CStateNode* next_node = NULL;

	nodes.emplace_back("spawn", this->TmpUpdate, this->IsAnimEnd, NULL);
	nodes.emplace_back("walk", this->TmpUpdate, this->DoAttack, NULL);*/
	PlayAni(0);
	
}

void CZombieStateMachine::Update(float time_elapsed)
{
	if (is_dead) {
		return;
	}
	float anim_time = time_elapsed;
	
	if (anim_state == ZombieAnimationState::WALK) {
		//anim_time = time_elapsed * (((CZombie*)object)->GetSpeed());
		//anim_time = time_elapsed * 0.5f;
	}

	if (blend_time < max_blend_time) {
		blend_time += time_elapsed;
	}

	CStateMachine::Update(anim_time);

	auto object_info = ((CZombie*)object)->GetZombie();
	float dead_time = 29.999f;

	switch (anim_state) {
	case ZombieAnimationState::ATTACK:
	case ZombieAnimationState::ATTACKED:
	case ZombieAnimationState::SPAWN:
		if (!is_anim_ended) {
			return;
		}
		else {
			object_info->_animation = ZombieAnimationState::WALK;
		}
		break;
	case ZombieAnimationState::DEAD:
		if (is_anim_ended) {
			is_dead = true;
			//model->UpdateNodeTM(dead_time, 0.0f, _playAniIdx);
			blend_mat = model->GetBoneMatLastFrame(ANIMATION_INDEX::DEAD);
			//blend_mat = model->GetBoneMat();
		}
		break;

		//if (!is_anim_ended) return;
		break;
	default:
		break;
	}

	if (anim_state == object_info->_animation) return;

	anim_state = object_info->_animation;
	
	switch (anim_state) {
	case ZombieAnimationState::ATTACK:
		ChangeAniWithBlend(ANIMATION_INDEX::ATTACK, model->GetBoneMat());
		break;
	case ZombieAnimationState::ATTACKED:
		ChangeAniWithBlend(ANIMATION_INDEX::ATTACKED, model->GetBoneMat());
		break;
	case ZombieAnimationState::SPAWN:
		ChangeAniWithBlend(ANIMATION_INDEX::SPAWN, model->GetBoneMat());
		break;
	case ZombieAnimationState::DEAD:
		ChangeAniWithBlend(ANIMATION_INDEX::DEAD, model->GetBoneMat());
		break;
	case ZombieAnimationState::IDLE:
		ChangeAniWithBlend(ANIMATION_INDEX::WALK, model->GetBoneMat());
		break;
	case ZombieAnimationState::WALK:
		ChangeAniWithBlend(ANIMATION_INDEX::WALK, model->GetBoneMat());
		break;
	}

}

void CZombieStateMachine::PrintAnim()
{
	map<ZombieAnimationState, string> anim_map;
	anim_map.emplace(ZombieAnimationState::ATTACK, "ATTACK");
	anim_map.emplace(ZombieAnimationState::ATTACKED, "ATTACKED");
	anim_map.emplace(ZombieAnimationState::SPAWN, "SPAWN");
	anim_map.emplace(ZombieAnimationState::DEAD, "DEAD");
	anim_map.emplace(ZombieAnimationState::WALK, "WALK");
	anim_map.emplace(ZombieAnimationState::IDLE, "IDLE");

	{
		auto& found = anim_map.find(anim_state);
		if (found == anim_map.end()) return;
		//cout << "anim state: " << (*found).second << endl;
	}
	{
		auto& found = anim_map.find(((CZombie*)object)->GetZombie()->_animation);
		if (found == anim_map.end()) return;
		//cout << "server anim state: " << (*found).second << endl;
	}
}

const array<XMMATRIX, 96>& CZombieStateMachine::GetBoneMat()
{
	if (is_dead) {
		return blend_mat;
	} 
	
	return CStateMachine::GetBoneMat();
}

//-----------------------------------------------------------------------------------------------
//p 0 - foward / 1 - left / 2 - right / 3 - backward / 4- shoot / 5- idle / 6 - attacked / 7 - dying
namespace PLAYER {
	enum PLAYER_ANIM_INDEX
	{
		FORWARD = 0,
		LEFT = 1,
		RIGHT = 2,
		BACKWARD = 3,
		WALK_AND_SHOOT = 4,
		IDLE = 5,
		ATTACKED = 6,
		DYING = 7,
		WALK_AND_RELOAD = 7
	};
}

CPlayerStateMachine::CPlayerStateMachine(CPlayer* object) 
	:CStateMachine(object->model, object)
{
	speed = object->speed;
	angle = object->move_angle;

	std::string name{ "spawn" };
	std::function<void(CStateMachine*, float)> func;
	std::function<bool(CStateMachine*)> state_change_func{ this->IsAnimEnd };
	CStateNode* next_node = NULL;

	//nodes.emplace_back("idle", this->TmpUpdate, this->IsWalking, NULL);
	//nodes.emplace_back("walk", this->TmpUpdate, this->IsAttacking, NULL);
}

void CPlayerStateMachine::Update(float time_elapsed)
{
	speed = object->speed;
	angle = object->move_angle;
	float anim_time = time_elapsed;
	if (is_dead) anim_time = 0.f;

	float pre_time = animation_time;

	if (anim_state == ClientAnimationState::WALK) {
		anim_time = time_elapsed * (speed / 1000.f);
	}
	else {
		CStateMachine::Update(anim_time);
	}

	auto object_info = ((CMainGamePlayer*)object)->GetPlayerInfo();

	switch (anim_state) {
	case ClientAnimationState::IDLE:
		object_info->_animation = ClientAnimationState::WALK;
		break;
	case ClientAnimationState::ATTACKED:
		if (is_anim_ended) {
			object_info->_animation = anim_state = ClientAnimationState::WALK;
		}
		break;
	case ClientAnimationState::DEAD:
		if (is_anim_ended) {
			is_dead = true;
			animation_time = pre_time;
			CStateMachine::Update(0.f);
		}
		if (object_info->hp > 0) {
			object_info->_animation = ClientAnimationState::WALK;
			is_dead = false;
		}
		break;
	default:
		break;
	}

	if (object_info->hp <= 0) {
		object_info->_animation = ClientAnimationState::DEAD;
	}

	if (anim_state == object_info->_animation) return;

	anim_state = object_info->_animation;
	switch (anim_state) {
	case ClientAnimationState::WALK_AND_SHOOT:
		PlayAni(PLAYER::WALK_AND_SHOOT);
		break;
	case ClientAnimationState::ATTACKED:
		PlayAni(PLAYER::ATTACKED);
		break;
	case ClientAnimationState::WALK_AND_RELOAD:
		PlayAni(PLAYER::WALK_AND_RELOAD);
		break;
	case ClientAnimationState::DEAD:
		PlayAni(PLAYER::DYING);
		break;
	case ClientAnimationState::IDLE:
		//anim_state = ClientAnimationState::WALK;
		object_info->_animation = ClientAnimationState::WALK;
		break;
	case ClientAnimationState::WALK:
		//model->PlayAni(PLAYER::WALK);
		break;
	}
}

void CPlayerStateMachine::PlayReloadAnim()
{
	/*switch (((CMainGamePlayer*)object)->GetPlayerInfo()->_animation) {
	case ClientAnimationState::ATTACKED:
	case ClientAnimationState::WALK_AND_RELOAD:
	case ClientAnimationState::DEAD:
		return;
		break;
	default:
		break;
	}*/
	/*anim_state = (ClientAnimationState)(((int)(anim_state) + 1) % 6);
	switch (anim_state) {
	case ClientAnimationState::WALK_AND_SHOOT:
		model->PlayAni(PLAYER::WALK_AND_SHOOT);
		cout << "WALK_AND_SHOOT" << endl;
		break;
	case ClientAnimationState::ATTACKED:
		model->PlayAni(PLAYER::ATTACKED);
		cout << "ATTACKED" << endl;
		break;
	case ClientAnimationState::WALK_AND_RELOAD:
		model->PlayAni(PLAYER::WALK_AND_RELOAD);
		cout << "WALK_AND_RELOAD" << endl;
		break;
	case ClientAnimationState::DEAD:
		model->PlayAni(PLAYER::DYING);
		cout << "DYING" << endl;
		break;
	case ClientAnimationState::IDLE:
		model->PlayAni(PLAYER::IDLE);
		cout << "IDLE" << endl;
		break;
	case ClientAnimationState::WALK:
		break;
	}*/
	//model->PlayAni(PLAYER::WALK_AND_RELOAD);
}

const array<XMMATRIX, 96>& CPlayerStateMachine::GetBoneMat()
{
	//return CStateMachine::GetBoneMat();
	Update(TIMEMANAGER.GetTimeElapsed());
	if (anim_state == ClientAnimationState::WALK) {
		return ((CBlendSkinModel*)model)->GetBoneMat(TIMEMANAGER.GetTimeElapsed(), speed, angle);
	}
	return model->GetBoneMat();
}


//-----------------------------------------------------------------
// state change functions

void CStateMachine::TmpUpdate(CStateMachine* machine, float time)
{
	return;
}

bool CStateMachine::IsAnimEnd(CStateMachine* machine)
{
	return machine->is_anim_ended;
}

bool CZombieStateMachine::DoAttack(CStateMachine* machine)
{
	if (((CZombieStateMachine*)machine)->anim_state == ZombieAnimationState::ATTACK) return true;
}

bool CPlayerStateMachine::IsWalking(CStateMachine* machine) {
	return ((CPlayerStateMachine*)machine)->speed > 10.0f;
}

bool CPlayerStateMachine::IsAttacking(CStateMachine* machine) {
	//return ((CMainGamePlayer*)machine->object) > 10.0f;
	return false;
}