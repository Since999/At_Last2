#pragma once

#include "common.h"
//#include "stdafx.h"
class Map;

class Client
{
public:
	int _id;
	char Name[MAX_NAME_SIZE];
	PlayerType _type;
	atomic_int hp, shp;
	int maxhp, maxshp;
	float speed;
	float max_speed;
	float acceleration = 30.0f;
	atomic_int bullet;
	float pre_x, pre_z;						// 플레이어 예전 위치, 패킷서 받아온 기존 위치
	atomic <float> x, z;						// 플레이어 현재 위치
	float t_x, t_z;								// 플레이어 방향
	float angle = 0.0f;							// 플레이어 방향
	float rotate_speed;				// 회전 속도
	bool is_input = false;
	float rotation_angle = 0.0f;
	float mx, mz;		// 마우스 클릭 위치
	atomic <ClientState> _state;
	atomic <ClientAnimationState> _animation;

	chrono::system_clock::time_point move_time;

	Direction _dir;
	//mutex hp_lock;
public:
	Client();
	~Client();

	float Get_Client_X() { return x; }
	float Get_Client_Z() { return z; }
	bool IsCollied(int r, int c, char map[WORLD_HEIGHT][WORLD_WIDTH]);
	void move(float time_elapsed);
	void ProcessInput(float x, float y);
};