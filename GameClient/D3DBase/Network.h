#pragma once

#include "Zombie.h"

#define ENABLE_NETWORK

struct BarricadePos {
	float x;
	float z;
	DIR dir;
};

enum class EVENT_TYPE : char
{
	PLAYER_MOVE
};

struct Timer_Event
{
	int p_id;
	chrono::high_resolution_clock::time_point start_time;
	EVENT_TYPE ev;

	constexpr bool operator < (const Timer_Event& _Left) const
	{
		return (start_time > _Left.start_time);
	}
};

class Network {
public:
	static Socket _socket;
	static array<Client, MAX_PLAYER> g_client;
	static int my_id;
	static int other_client_id1;
	static int other_client_id2;
	static int _prev_size;
	static bool select_complete;
	static bool login_complete;
	static atomic_bool barricade_req;
	static atomic_bool game_start;

	static MapType map_type;

	static BarricadePos one_barricade[42];
	static BarricadePos two_barricade[32];
	static BarricadePos three_barricade[30];
	static BarricadePos three_barricade2[30];

	static array<Zombie, ROAD_ZOMBIE_NUM> r_zombie1;
	static array<Zombie, ROAD_ZOMBIE_NUM> r_zombie2;
	static array<Zombie, ROAD_ZOMBIE_NUM> r_zombie3;

	static array<Zombie, FIRST_CHECK_POINT_ZOMBIE_NUM> b_zombie1;
	static array<Zombie, TWO_CHECK_POINT_ZOMBIE_NUM> b_zombie2;
	static array<Zombie, THREE_CHECK_POINT_ZOMBIE_NUM> b_zombie3;

	static bool key_down_state;
	static bool mouse_state;
	static bool attack_state;

	static chrono::system_clock::time_point key_down_time;
	static chrono::system_clock::time_point mouse_time;
	static chrono::system_clock::time_point total_time;
	static chrono::system_clock::time_point attack_time;

	static atomic_int fps_rate;

	//static mutex move_lock;
	static mutex id_lock;
	//static mutex bullet_lock;
	//static mutex state_lock;
	//static mutex select_lock;

	vector<thread> worker_threads;
	static priority_queue <Timer_Event> timer_queue;
	static mutex timer_lock;

	static char map[WORLD_HEIGHT][WORLD_WIDTH];
public:
	Network();
	~Network();
	
	static void Initialize();
	static void ReadMapFile();
	static void send_login_packet(char* str);
	static void send_player_select_packet(PlayerType type);
	static void ProcessPacket(unsigned char* ptr);
	static void volatile ProcessData(Exp_Over& exp_over, int &size);
	static void Work();
	static void send_player_move_packet(float t_x, float t_z, float speed, float x, float z);
	static void Do_Timer();
	static void Send_request_packet(MsgType type);
	static void Send_commander_special_req_packet(int c_id);
	static void Send_commander_special_change_packet(int c_id);
	static void Send_chat_packet(char* msg);
	static void Send_attack_packet(int m_x, int m_z);
	static void Send_rotate_packet(float m_x, float m_z);
	static BarricadePos Change_Client_Pos(iPos pos);
	static void PlayerMove(int p_id);
	static void AddTimer(int id, EVENT_TYPE type, int duration);
	static void Update(float time_elapsed);
	static void ZombieMove(Zombie& zombie, float time_elapsed);
	static void ZombieAngle(Zombie& zombie, float time_elapsed);
};