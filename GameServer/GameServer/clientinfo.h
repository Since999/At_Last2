#pragma once

#include "map.h"

class Player
{
public:
	atomic_short hp, maxhp;			// �ӽ� short
	char attack;
	char interaction;
	char special_skill;
	atomic_char bullet;
	float x, z;
	float m_x, m_z;
	float speed;
	DIR special_dir;
	Direction dir;

public:
	Player();
	virtual ~Player();

	bool Move(Direction dir, Map& map);
	bool IsCollied(int r, int c, Map& map); 
	bool PlayerAttack(float x1, float z1, float z_x, float z_z); // �÷��̾� x, z �� ������ ���� x1, z1 �̵������� ������ �����Ŀ� ������ ��ǥ �ֱ� -> �׷��� ���� ��ǥ�� �浹�Ǹ� true �ƴϸ� false
};

class Client
{
public:
	char player_name[MAX_NAME_SIZE];
	int _id;
	
	//mutex state_lock;
	//mutex type_lock;
	//mutex start_lock;
	//mutex map_lock;
	//mutex attack_lock;
	//mutex bullet_lock;
	//mutex hp_lock;
	
	mutex move_lock;
	mutex list_lock;
	mutex size_lock;
	mutex send_lock;

	atomic_bool send_start_packet;

	atomic <MapType> map_type;
	atomic <ClientState> _state;
	atomic <PlayerType> _type;
	Exp_Over _recv_over;
	Socket m_socket;
	Player* player;
	atomic_int prev_size;

	unordered_set<int> zombie_list;

	unsigned char zombie_send_buf[MAX_BUFFER_SIZE];
	atomic_int _zombie_prev_size;
	atomic_bool _zombie_send_overflow;

	chrono::system_clock::time_point mouse_click_time;
	chrono::system_clock::time_point idle_time;

	int server_time;
public:
	Client();
	~Client();

	void do_send(int num_bytes, void* mess);
	void do_recv();
};

class Commander : public Player
{
public:

public:
	Commander();
	~Commander();
};

class Engineer : public Player
{
public:

public:
	Engineer();
	~Engineer();
};

class Mercynary : public Player
{
public:

public:
	Mercynary();
	~Mercynary();
};