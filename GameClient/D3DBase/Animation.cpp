#include "stdafx.h"
#include "Animation.h"

Animation::Animation()
{
}

Animation::~Animation()
{
}


bool Animation::UpdateAnimation(float& time, float time_elapsed)
{
	bool result = false;
	//Calc Frame Cnt
	if (_playState == PLAY_STATE::STOP) {
		return true;
	}

	//float _playTime = fmod(time, _duration);
	time += _tickPerSecond * time_elapsed;

	if (abs(time - curr_node_time) < 0.00001f) return false;
	//프레임 시간 초기화
	while (time > _duration) {
		time -= _duration;
		if (!_isRepeat) {
			time = _duration -0.1f;
			//_playState = PLAY_STATE::STOP;
			//return true;
		}
		result = true;
	}
	
	_playTime = time;
	
	//아핀 변환 정보로 TM행렬을 만들기 위해 사용되는 참조 변수
	XMVECTOR Z = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	//AniNode Update
	for (auto& node : _aniNode) {
		//현재 프레임 시간이 첫 번째 키 프레임 시간보다 빠른 경우
		if (node.keyFrame.front().timePos >= _playTime) {
			XMVECTOR T = XMLoadFloat3(&node.keyFrame.front().trans);
			XMVECTOR S = XMLoadFloat3(&node.keyFrame.front().scale);
			XMVECTOR R = XMLoadFloat4(&node.keyFrame.front().rotation);

			node.aniTM = XMMatrixAffineTransformation(S, Z, R, T);
		}
		//현재 프레임 시간이 마지막 키 프레임 시간보다 빠른 경우
		else if (node.keyFrame.back().timePos <= _playTime) {
			XMVECTOR T = XMLoadFloat3(&node.keyFrame.back().trans);
			XMVECTOR S = XMLoadFloat3(&node.keyFrame.back().scale);
			XMVECTOR R = XMLoadFloat4(&node.keyFrame.back().rotation);

			node.aniTM = XMMatrixAffineTransformation(S, Z, R, T);
		}
		else {
			for (UINT i = 0; i < node.keyFrame.size() - 1; i++) {
				auto key = node.keyFrame;
				if (_playTime >= key[i].timePos && _playTime <= key[i + 1].timePos) {
					float lerpRate = (_playTime - key[i].timePos) / (key[i + 1].timePos - key[i].timePos);

					XMVECTOR s1 = XMLoadFloat3(&key[i].scale);
					XMVECTOR s2 = XMLoadFloat3(&key[i + 1].scale);

					XMVECTOR t1 = XMLoadFloat3(&key[i].trans);
					XMVECTOR t2 = XMLoadFloat3(&key[i + 1].trans);

					XMVECTOR r1 = XMLoadFloat4(&key[i].rotation);
					XMVECTOR r2 = XMLoadFloat4(&key[i + 1].rotation);

					//두 프레임 사이 정보 보간
					XMVECTOR T = XMVectorLerp(t1, t2, lerpRate);
					XMVECTOR S = XMVectorLerp(s1, s2, lerpRate);
					XMVECTOR R = XMQuaternionSlerp(r1, r2, lerpRate);

					node.aniTM = XMMatrixAffineTransformation(S, Z, R, T);
					break;
				}
			}
		}
	}
	curr_node_time = _playTime;
	return result;
}

#include <algorithm>
//노드 정보에 맞는 ANI 행렬 정보 반환
void Animation::GetAniTM(wstring nodeName, XMMATRIX &tm)
{
	XMMATRIX aniTM = tm;

	//일치하는 노드 탐색
	//auto res = std::find_if(_aniNode.begin(), _aniNode.end(), [&](AniNode aniNode) {	return nodeName == aniNode.name; });
	auto res = std::lower_bound(_aniNode.begin(), _aniNode.end(), nodeName, [](const AniNode& aniNode, const wstring& nodeName ) { return aniNode.name < nodeName; });
	
	//있는 경우 반환, 없으면 들어온 행렬정보(로컬행렬) 그대로 반환
	//애니메이션 행렬은 노드의 로컬 정보를 포함하고 있기 때문에 노드의 로컬 행렬이 된다.
	
	if (res == _aniNode.end()) return;
	if ((*res).name != nodeName) return;

	tm = res->aniTM;

	return;
}
