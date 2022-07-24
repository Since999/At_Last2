#pragma once

#include "zombieinfo.h"
#include <chrono>

enum class EVENT_TYPE : char
{
	EVENT_DOOR_OPEN,
	EVENT_NPC_SPAWN,
	EVENT_NPC_MOVE,
	EVENT_NPC_ATTACK,
	EVENT_NPC_SEND,
	EVENT_NPC_DEAD
};

struct timer_event {
	int obj_id;
	chrono::high_resolution_clock::time_point start_time;
	EVENT_TYPE ev;
	int target_id;
	constexpr bool operator < (const timer_event& _Left) const
	{
		return (start_time > _Left.start_time);
	}

};

// static 하기 싫다면
// 1. 아예 전역 변수로 하거나?
// 2. 싱글톤(멀티 쓰레드에서는 주의가 필요, 피닉스 싱글톤? 레이징_이니셜라이저 아니면 static 만들고 메인쓰레드가 소유를 가지면 됨
// 3. context class로 만들고 동적할당 이거에대한 포인터 또는 레퍼런스를 모든 쓰레드에서 동일하게 가지면 된다.

class Server
{
public:
	static Socket _socket;
	static Exp_Over* _exp_over;

	static Map map;
	static MapType map_type;
	static AS_Map as_map;

	static array<Client, MAX_PLAYER> g_clients;

	static array<NPC, ROAD_ZOMBIE_NUM> r_zombie1;
	static array<NPC, ROAD_ZOMBIE_NUM> r_zombie2;
	static array<NPC, ROAD_ZOMBIE_NUM> r_zombie3;

	static array<NPC, FIRST_CHECK_POINT_ZOMBIE_NUM> b_zombie1;
	static array<NPC, TWO_CHECK_POINT_ZOMBIE_NUM> b_zombie2;
	static array<NPC, THREE_CHECK_POINT_ZOMBIE_NUM> b_zombie3;

	vector<thread> worker_threads;

	static priority_queue <timer_event> timer_queue;
	static mutex timer_lock;
	static mutex num_lock;
	static mutex spawn_lock;
	static mutex map_lock;

	static float s_speed;
	static float z_speed;
	static chrono::milliseconds sec;

	static bool zombie_send;
	static bool game_start;
	static int door_num;
	static int remain_zombie_num;

	static CGameTimer game_timer;
public:
	Server();
	~Server();

	static void Initialize();
	static void Work();
	void Update();
	static int NewID();
	static bool NCDis_check(int c_id,  NPC& npc);
	static void Disconnect(int c_id);
	static void ProcessPacket(int client_id, unsigned char* p);
	static void Send_login_ok_packet(int c_id);
	static void Send_barricade_packet(int c_id);
	static void Send_game_start_packet(int c_id);
	static void Send_select_packet(int c_id, int s_id);
	static void Send_player_move_packet(int c_id, int s_id, float x, float z,  float t_x, float t_z, float speed, float rotation, bool input);
	static void Send_player_attack_packet(int c_id, int a_id);
	static void Send_player_dead_packet(int c_id, int d_id);
	static void Send_player_info_packet(int c_id, int s_id, short hp);
	static void Send_player_reload_packet(int c_id, int s_id);
	static void Send_player_rotate_packet(int c_id, int s_id, float m_x, float m_z);
	static void Send_player_idle_packet(int c_id, int s_id);
	static void Send_player_zombie_kill_num_packet(int c_id, int s_id, int z_num);
	static void Send_commander_skill_packet(int c_id, int s_id);
	static void Send_commander_skill_check_packet(int c_id, int s_id);
	static void Send_engineer_skill_packet(int c_id,int s_id, int t_x, int t_z);
	static void Send_engineer_skill_check_packet(int c_id, int x, int z);
	static void Send_viewlist_put_packet(int c_id, int z_id, MapType m_type, float z_x, float z_z, MsgType msg, ZombieType z_type);
	static void Send_viewlist_remove_packet(int c_id, int z_id, MapType m_type);
	static void Send_search_packet(int c_id, int x, int z, ObjectType _type);
	static void Send_fail_packet(int c_id, MsgType type);
	static void Send_door_open_packet(int c_id, int s_id, int row, int col, int size_x, int size_z);
	static void Send_zombie_spawn_packet(int c_id, int z_id, float x, float z, ZombieType type, int hp, float angle);
	static void Send_zombie_move_packet(int c_id, int z_id, float x, float z, MapType m_type, float t_x, float t_z, float speed, Direction dir);
	static void Send_zombie_info_packet(int c_id, int z_id, int hp, MapType m_type);
	static void Send_zombie_dead_packet(int c_id, int z_id, MapType m_type);
	static void Send_zombie_search_packet(int c_id, int s_id, int z_id, MapType m_type);
	static void Send_zombie_attack_packet(int c_id, int z_id, MapType m_type);
	static void Send_zombie_arrive_packet(int c_id, int z_id, MapType m_type, Direction dir);
	static void Send_zombie_number_packet(int c_id, int z_num);
	static void Send_zombie_all_kill_packet(int c_id, MapType m_type);
	static void Send_gm_change_map_packet(int c_id, int s_id, int x, int z);
	static void Send_gm_hp_packet(int c_id, int s_id);
	static void AddTimer(int z_id, EVENT_TYPE type, int duration);
	static void Do_Timer();
	static void ChangeMapType(Client& cl);
	static void PlaceZombie(MapType m_type);
	static void InitZombie(NPC& npc, int &i, float &PosX, float &PosZ);
	static void ChangeRoadZombieStateToSpawn(NPC& npc);
	static void ChangeZombieStateToSpawn(int spawn_id);
	static bool MapCheck(MapType map_type);
	static void PlayerAttack(Client& cl, NPC& npc, MapType m_type, float p_x, float p_z);
	static void ResurrectionPlayer(Client& cl);
	static void CommanderSpecialSkill(Client& cl);
	static void EngineerSpecialSkill(Client& cl);
	static bool EngineerSpecialSkillMapCheck(int x, int z, Direction dir);
	static bool EngineerSpecialSkillZombieCheck(int x, int z, Direction dir, NPC& npc);
	static void EngineerBuildBarricade(int bx, int bz, Direction dir);
	static void MercenarySpecialSkill(Client& cl);
	static void DieZombie(Zombie* zombie) { delete zombie; zombie = nullptr; };
	static float Distance(float s_x, float s_z, float e_x, float e_z);
	static void SearchZombieAstar(int col, int row, Client& cl, NPC& npc);
	static void ZombieMove(int z_id);
	static void ZombieAstarMove(NPC& npc, MapType m_type, iPos start_pos, iPos end_pos);
	static void ZombieDead(NPC& npc, MapType m_type);
	static void ZombieAllKill(NPC& npc);
	static void ZombieAttack(int z_id);
	static void ZombiePlayerAttack(NPC& npc, MapType m_type);
	static bool ZombieAttackRangeCheck(Direction dir, float att_range, float x, float z, float c_x, float c_z);
	static void ZombieSend();
	static bool ZombieRemain( NPC& npc);
	static float ZombieSetAngle(NPC& npc, Client& cl);
	static bool ZombieSendInsert(int c_id, void* packet, int size);
};