//====================================================================================
//		## Animation ## (애니메이션과 관련된 구조체와 클래스)
//====================================================================================
#pragma once
#include "stdafx.h"
enum class PLAY_STATE {
	PLAY, STOP
};

//키 프레임 정보
struct KeyFrame {
	float		timePos		= 0.f;
	XMFLOAT3	trans		= XMFLOAT3(0.f,0.f,0.f);
	XMFLOAT4	rotation	= XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	XMFLOAT3	scale		= XMFLOAT3(0.f, 0.f, 0.f);
};

//본과 열결된 애니메이션 정보
struct AniNode {
	vector<KeyFrame>	keyFrame;							//키프레임 리스트
	wstring				name	= L"none";					//연결된 본 이름

	XMMATRIX			aniTM	= XMMatrixIdentity();		//애니메이션 행렬, 프레임에 맞춰 업데이트된 행렬 정보를 저장
};


//애니메이션 클래스
class Animation
{
//*************************************** 멤버 함수 ************************************************//
public:
	Animation();
	~Animation();

	//Animation Play Function
	void				Play()										{	_playState	= PLAY_STATE::PLAY;	}
	void				Pause()										{	_playState	= PLAY_STATE::STOP;	}
	void				Stop()										{	_playState	= PLAY_STATE::STOP;	}

	//반복 재생 상태 체크
	bool				IsRepeat()									{	return _isRepeat;				}

	//Set Function
	void				SetTickPerSecond(float tps)					{	_tickPerSecond = tps;			}		//틱 값 셋팅
	void				SetDuration(float duration)					{	_duration = duration;			}		//진행속도 셋팅
	void				SetLastFrame(float time)					{	_lastTime = time;				}		//마지막 프레임 셋팅
	void				SetName(wstring name)						{	_name = name;					}		//애니메이션 이름
	void				TurnRepeat()								{	_isRepeat = !_isRepeat;			}		//반복재생 전환
	void				SetRepeat(bool re) { _isRepeat = re; }


	//Get Function
	vector<AniNode>&	GetAniNodeList()							{	return _aniNode;				}		//애니메이션 노드 리스트 반환
	void				GetAniTM(wstring nodeName, XMMATRIX &tm);												//노드에 맞는 애니메이션 행렬을 반환
	wstring				GetName()									{	return _name;					}		//애니메이션 이름 반환
	
	float GetTickPerSecond() { return _tickPerSecond; }

	//Update AniTM
	bool				UpdateAnimation(float& t, float time_elapsed);					//애니메이션을 업데이트한다.
	bool				UpdateAnimationLastFrame();


//*************************************** 멤버 변수 ***********************************************//
private:
	vector<AniNode>		_aniNode;										//애니메이션 노드 리스트
	
	wstring				_name			= L"none";						//이름
	float				_tickPerSecond	= 0.f;							//tick 정보
	float				_duration		= 0.f;							//속도
	float				_lastTime		= 0.f;							//마지막 프레임

	float				curr_node_time	= -1.f;

	bool				_isRepeat		= true;							//재생 완료시 반복 재생 플래스
	PLAY_STATE			_playState		= PLAY_STATE::PLAY;				//재생 상태

	float				_playTime =		0.f;
};
