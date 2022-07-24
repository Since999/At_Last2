#pragma once

#include "AstarAlgoritm.h"

class Zombie
{
public:
	atomic <ZombieType> _type;
	atomic_char hp;
	char damage;
	float attRange, speed, infection;
	float searchRange;
	float x, z;
	float angle;

	chrono::milliseconds attack_timer;						// 공격 주기

	stack<AS_Node*> root;			// AStarAI 를 통해 얻은 내용을 여기에 저장할 것
	AStarAI astar;
	Direction zombie_dir;
	AS_Node add_node;

	mutex z_move_lock;
	//mutex z_speed_lock;
	//mutex z_attack_lock;
public:
	Zombie();
	virtual ~Zombie();

	void SetX(float pos_x) { x = pos_x; };
	void SetZ(float pos_z) { z = pos_z; };
	float GetX() { return x; };
	float GetZ() { return z; };

	MoveResult Move(float speed, Map& map);
	bool IsCollied(int r, int c, Map& map);
	Direction RootDir(float s_x, float s_z, float e_x, float e_z);
};

class NPC
{
public:
	int _id;
	Zombie* zombie;
	atomic <ZombieState> _state;
	
	//mutex state_lock;
	//mutex search_lock;
	mutex time_lock;

	atomic_bool map_check;
	atomic_bool astar_check;
	atomic_bool search_check;

	chrono::system_clock::time_point attack_delay_time;
public:
	NPC();
	~NPC();
};

class NormalZombie : public Zombie
{
public:
	NormalZombie();
	~NormalZombie();
};

class SoldierZombie : public Zombie
{
public:
	SoldierZombie();
	~SoldierZombie();
};

class TankerZombie : public Zombie
{
public:
	TankerZombie();
	~TankerZombie();
};

class DogZombie : public Zombie
{
public:
	DogZombie();
	~DogZombie();
};