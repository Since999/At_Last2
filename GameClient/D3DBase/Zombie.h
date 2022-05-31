#pragma once

#include "Client.h"

class Zombie
{
public:
	ZombieType _type;
	atomic <float> hp;
	atomic <float> x, z;
	int _id;
	ZombieState _state;
	atomic_bool view;				// view -> true ���� , false �Ⱥ���
	atomic <ZombieAnimationState> _animation;
	atomic <float> angle;
	float t_x, t_z;
	float speed;
	Direction dir;
	atomic_bool arrive;
	//mutex hp_lock;
public:
	Zombie();
	~Zombie();
	bool IsCollied(int r, int c, char map[WORLD_HEIGHT][WORLD_WIDTH]);
};