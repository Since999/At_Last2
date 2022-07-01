#include "Network.h"
#include "AnimationShader.h"

Socket Network::_socket;
array<Client, MAX_PLAYER> Network::g_client;
int Network::_prev_size;
int Network::my_id;
bool Network::select_complete;
bool Network::login_complete;
atomic_bool Network::game_start;
atomic_bool Network::barricade_req;
chrono::system_clock::time_point Network::total_time;
bool Network::key_down_state;
bool Network::mouse_state;
bool Network::attack_state;
chrono::system_clock::time_point Network::mouse_time;
chrono::system_clock::time_point Network::attack_time;
chrono::system_clock::time_point Network::key_down_time;
int Network::other_client_id1;
int Network::other_client_id2;

MapType Network::map_type;

array<Zombie, ROAD_ZOMBIE_NUM> Network::r_zombie1;
array<Zombie, ROAD_ZOMBIE_NUM> Network::r_zombie2;
array<Zombie, ROAD_ZOMBIE_NUM> Network::r_zombie3;

array<Zombie, FIRST_CHECK_POINT_ZOMBIE_NUM> Network:: b_zombie1;
array<Zombie, TWO_CHECK_POINT_ZOMBIE_NUM> Network::b_zombie2;
array<Zombie, THREE_CHECK_POINT_ZOMBIE_NUM> Network:: b_zombie3;

BarricadePos Network::one_barricade[42];
BarricadePos Network::two_barricade[32];
BarricadePos Network::three_barricade[30];
BarricadePos Network::three_barricade2[30];

atomic_int Network::fps_rate;

//mutex Network::move_lock;
mutex Network::id_lock;
//mutex Network::bullet_lock;
//mutex Network::state_lock;
//mutex Network::select_lock;

priority_queue <Timer_Event> Network::timer_queue;
mutex Network::timer_lock;

char Network::map[WORLD_HEIGHT][WORLD_WIDTH];

Network::Network()
{
	
}

Network::~Network() 
{

}

void Network::ReadMapFile()
{
	ifstream in("no_door_map2.txt");

	for (int i = 0; i < WORLD_HEIGHT; ++i)
		for (int j = 0; j < WORLD_WIDTH; ++j)
			in >> map[i][j];

	in.close();
}

void Network::Initialize()
{
	ReadMapFile();

	map_type = MapType::SPAWN;

	select_complete = false;
	login_complete = false;
	game_start = false;
	barricade_req = false;
	key_down_state = false;
	total_time = chrono::system_clock::now();
	key_down_time = chrono::system_clock::now();

	other_client_id1 = -1;
	other_client_id2 = -1;

	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	int retval = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (retval != 0)
		throw Exception("Start Fail");

	_socket.Init();
	_socket.CreatePort();
	_socket.Connect(SERVER_IP, SERVER_PORT);
}

void Network::AddTimer(int id, EVENT_TYPE type, int duration)
{
	timer_lock.lock();
	Timer_Event te{ id, chrono::high_resolution_clock::now() + chrono::milliseconds(duration), type };
	timer_queue.push(te);
	timer_lock.unlock();
}

void Network::Send_request_packet(MsgType type)
{
	cs_request_packet packet;
	packet.size = sizeof(packet);
	packet.type = type;
	_socket.do_send(sizeof(packet), &packet);
}

BarricadePos Network::Change_Client_Pos(iPos pos)
{
	BarricadePos temp;
	temp.x = (pos.x - 550.0f) * (-100.0f);
	temp.z = (pos.z - 210.0f) * (-100.0f);
	temp.dir = pos.dir;
	return temp;
}

void Network::Send_attack_packet(int m_x, int m_z)
{
	cs_attack_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_PLAYER_ATTACK;
	packet.x = m_x;
	packet.z = m_z;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::Send_rotate_packet(float m_x, float m_z)
{
	cs_rotate_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_PLAYER_ROTATE;
	packet.mx = m_x;
	packet.mz = m_z;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::ProcessPacket(unsigned char* ptr)
{
	int packet_type = (int)ptr[2];

	switch (packet_type)
	{
	case (int)MsgType::SC_LOGIN_OK:
	{
		sc_login_ok_packet* packet = reinterpret_cast<sc_login_ok_packet*>(ptr);

		my_id = packet->id;
		g_client[my_id]._id = packet->id;
		strcpy_s(g_client[my_id].Name, packet->name);
		g_client[my_id]._state = ClientState::INGAME;

		g_client[my_id].move_time = chrono::system_clock::now();

		g_client[my_id]._animation = ClientAnimationState::IDLE;

		login_complete = true;

		break;
	}
	case (int)MsgType::SC_LOGIN_FAIL:
	{
		login_complete = false;

		break;
	}
	case (int)MsgType::SC_LOGIN_OTHER:
	{
		sc_login_other_packet* packet = reinterpret_cast<sc_login_other_packet*>(ptr);
		int id = packet->id;

		if (other_client_id1 == -1)
		{
			id_lock.lock();
			g_client[id]._id = packet->id;
			strcpy_s(g_client[id].Name, packet->name);
			other_client_id1 = id;
			g_client[other_client_id1]._state = ClientState::INGAME;

			g_client[other_client_id1]._animation = ClientAnimationState::IDLE;
			id_lock.unlock();

			cout << "다른 클라1 : " << other_client_id1 << "\n";
			break;
		}
		if (other_client_id2 == -1 && other_client_id1 != id)
		{
			id_lock.lock();
			g_client[id]._id = packet->id;
			strcpy_s(g_client[id].Name, packet->name);
			other_client_id2 = id;
			g_client[other_client_id2]._state = ClientState::INGAME;

			g_client[other_client_id2]._animation = ClientAnimationState::IDLE;
			id_lock.unlock();

			cout << "다른 클라2 : " << other_client_id2 << "\n";
			break;
		}
		break;
	}
	case (int)MsgType::SC_BARRICADE_SEND:
	{
		sc_barricade_packet* packet = reinterpret_cast<sc_barricade_packet*>(ptr);

		int i = 0;
		for (auto& bar : one_barricade)
		{
			bar = Change_Client_Pos(packet->one_base[i]);
			map[packet->one_base[i].z][packet->one_base[i++].x] = (char)MazeWall::BARRICADE;
		}

		i = 0;
		for (auto& bar : two_barricade)
		{
			bar = Change_Client_Pos(packet->two_base[i]);
			map[packet->two_base[i].z][packet->two_base[i++].x] = (char)MazeWall::BARRICADE;
		}

		i = 0;
		for (auto& bar : three_barricade)
		{
			bar = Change_Client_Pos(packet->three_base[i]);
			map[packet->three_base[i].z][packet->three_base[i++].x] = (char)MazeWall::BARRICADE;
		}

		i = 0;
		for (auto& bar : three_barricade2)
		{
			bar = Change_Client_Pos(packet->three_base2[i]);
			map[packet->three_base2[i].z][packet->three_base2[i++].x] = (char)MazeWall::BARRICADE;
		}

		Send_request_packet(MsgType::CS_GAME_START_REQUEST);
		barricade_req = true;
		// packet.onebase -> 1거점 미로 시작 좌표, 방향
		// packet.twobase -> 2거점 미로 시작 좌표, 방향
		// packet.threebase -> 3거점1 미로 시작 좌표, 방향
		// packet.threebase2 -> 3거점2 미로 시작 좌표, 방향
		/*
		iPos라는 구조체로 만들어져 있으며, x, z, DIR구조체(width, height)로 이루어져 있습니다.
		iPos pos;
		pos.x = packet.onebase[0].x;
		pos.z = packet.onebase[0].z;
		pos.dir = packet.onebase[0].dir;
		이런식으로 복사해 오시거나 참조해가시면 될듯합니다.
		dir 이 width 이면 x,z좌표 기준으로 좌측 2, 우측 2칸 5칸
		dir 이 height 이면 x,z 좌표 기준으로 상 2, 하 2칸 5칸 입니다.
		*/

		break;
	}
	case (int)MsgType::SC_GAME_START:
	{
		cout << "게임 시작 \n";
		game_start = true;
		Send_request_packet(MsgType::CS_GAME_START);

		AddTimer(my_id, EVENT_TYPE::PLAYER_MOVE, 1000);
		break;
	}
	case (int)MsgType::SC_GAME_START_FAIL:
	{
		// 버퍼링 또는 로딩중 표시.
		// 게임 시작했는지에 대한 타이머 질문에 대한 서버에서 아직 안됬다고 대답

		//cout << "게임을 시작하는데 실패하였습니다. \n";
		break;
	}
	case (int)MsgType::SC_PLAYER_ATTACK:
	{
		
		break;
	}
	case (int)MsgType::SC_PLAYER_ROTATE:
	{
		sc_player_rotate_packet* packet = reinterpret_cast<sc_player_rotate_packet*>(ptr);

		g_client[packet->id].mx = packet->mx;
		g_client[packet->id].mz = packet->mz;

		break;
	}
	case (int)MsgType::SC_PLAYER_RELOAD_REQUEST:
	{
		// 발사가 안되어 딸각 딸각 하거나, 재장전 메시지를 출력하면 좋을듯
		cout << "총알이 모두 떨어져 재장전 해야합니다! \n";

		break;
	}
	case (int)MsgType::SC_PLAYER_RELOAD:
	{
		sc_player_reload_packet* packet = reinterpret_cast<sc_player_reload_packet*>(ptr);

		g_client[packet->id].bullet = packet->bullet;

		break;
	}
	case (int)MsgType::SC_PLAYER_BULLET_INFO:
	{
		sc_player_bullet_info_packet* packet = reinterpret_cast<sc_player_bullet_info_packet*>(ptr);

		cout << (int)packet->id << "의 총알이 " << (int)packet->bullet << "개 있습니다 \n";

		//bullet_lock.lock();
		g_client[packet->id].bullet = packet->bullet;
		//bullet_lock.unlock();

		break;
	}
	case (int)MsgType::SC_PLAYER_IDLE:
	{
		sc_player_idle_packet* packet = reinterpret_cast<sc_player_idle_packet*>(ptr);

		//g_client[packet->id]._animation = ClientAnimationState::IDLE;
		break;
	}
	case (int)MsgType::SC_PLAYER_BUILD:
	{
		break;
	}
	case (int)MsgType::SC_PLAYER_CHAT:
	{
		sc_player_chat_packet* packet = reinterpret_cast<sc_player_chat_packet*>(ptr);

		cout << "[ " << g_client[packet->s_id].Name << " ] 이 보냄 : " << packet->message << "\n";

		break;
	}
	case (int)MsgType::SC_PLAYER_INTERATION:
	{
		break;
	}
	case (int)MsgType::SC_DOOR_OPEN:
	{
		sc_door_open_packet* packet = reinterpret_cast<sc_door_open_packet*>(ptr);

		cout << packet->row << ", " << packet->col << "에 있는 문이 열렸습니다 \n";

		break;
	}
	case (int)MsgType::SC_PLAYER_NOT_ENGINEER:
	{
		cout << "당신은 엔지니어가 아니라 상호작용할 수 없습니다 \n";

		break;
	}
	case (int)MsgType::SC_PLAYER_MOVE:
	{
		sc_player_move_packet* packet = reinterpret_cast<sc_player_move_packet*>(ptr);

		int id = packet->id;

		g_client[id].x = packet->x;
		g_client[id].z = packet->z;
		g_client[id].t_x = packet->t_x;
		g_client[id].t_z = packet->t_z;
		g_client[id].speed = packet->speed;

		break;
	}
	case (int)MsgType::SC_PLAYER_MOVE_FAIL:
	{
		cout << "움직이는데에 실패하였습니다 \n";

		break;
	}
	case (int)MsgType::SC_PLAYER_SELECT:
	{
		sc_player_select_packet* packet = reinterpret_cast<sc_player_select_packet*>(ptr);

		int id = packet->id;

		g_client[id]._type = packet->playertype;
		g_client[id].hp = packet->hp;
		g_client[id].maxhp = packet->maxhp;
		g_client[id].shp = packet->shp;
		g_client[id].maxshp = packet->maxshp;
		g_client[id].x = packet->x;
		g_client[id].z = packet->z;
		g_client[id].bullet = packet->bullet;
		//g_client[id].speed = packet->speed;
		g_client[id].max_speed = packet->speed;;

		cout << id << " : x : " << packet->x << ", z : " << packet->z << "\n";

		break;
	}
	case (int)MsgType::SC_PLAYER_SELECT_FAIL:
	{
		// 플레이어가 캐릭터 선택에 실패했다는 메시지 출력이 있으면 좋을 것 같음
		cout << "플레이어가 이미 선택되었습니다 \n";

		sc_fail_packet* packet = reinterpret_cast<sc_fail_packet*>(ptr);

		if(g_client[packet->id]._type == PlayerType::NONE)
			select_complete = false;

		break;
	}
	case (int)MsgType::SC_PLAYER_SPECIAL:
	{
		break;
	}
	case (int)MsgType::SC_PLAYER_DEAD:
	{
		sc_player_dead_packet* packet = reinterpret_cast<sc_player_dead_packet*>(ptr);

		cout << (int)packet->id << "가 죽었습니다 \n";
		//state_lock.lock();
		g_client[packet->id]._state = ClientState::DEAD;
		//state_lock.unlock();
		break;
	}
	case (int)MsgType::SC_PLAYER_SEARCH:
	{
		sc_search_packet* packet = reinterpret_cast<sc_search_packet*>(ptr);

		cout << "x : " << packet->x << ", z : " << packet->z << "에 " << (char)packet->obj_type <<  "이 있습니다. \n";
		break;
	}
	case (int)MsgType::SC_UPDATE_OBJECT_INFO:
	{
		break;
	}
	case (int)MsgType::SC_UPDATE_PLAYER_INFO:
	{
		sc_player_hp_packet* packet = reinterpret_cast<sc_player_hp_packet*>(ptr);

		cout << (int)packet->id << "의 체력이 " << (int)packet->hp << "로 변경되었습니다 \n";

		//g_client[packet->id].hp_lock.lock();
		g_client[packet->id].hp = packet->hp;
		//g_client[packet->id].hp_lock.unlock();

		break;
	}
	case (int)MsgType::SC_UPDATE_STATE:
	{
		break;
	}
	case (int)MsgType::SC_UPDATE_ZOMBIE_INFO:
	{
		sc_update_zombie_info_packet* packet = reinterpret_cast<sc_update_zombie_info_packet*>(ptr);

		int id = packet->id;

		cout << (int)packet->id << "의 체력이 " << (int)packet->hp << "로 바뀌었습니다 \n";

		switch (packet->map_type)
		{
		case MapType::FIRST_PATH:
		{
			//r_zombie1[id].hp_lock.lock();
			r_zombie1[id].hp = packet->hp;
			r_zombie1[id]._animation = ZombieAnimationState::ATTACKED;
			//r_zombie1[id].hp_lock.unlock();
			break;
		}
		case MapType::SECOND_PATH:
		{
			//r_zombie2[id].hp_lock.lock();
			r_zombie2[id].hp = packet->hp;
			r_zombie2[id]._animation = ZombieAnimationState::ATTACKED;
			//r_zombie2[id].hp_lock.unlock();
			break;
		}
		case MapType::FINAL_PATH:
		{
			//r_zombie3[id].hp_lock.lock();
			r_zombie3[id].hp = packet->hp;
			r_zombie3[id]._animation = ZombieAnimationState::ATTACKED;
			//r_zombie3[id].hp_lock.unlock();
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			//b_zombie1[id].hp_lock.lock();
			b_zombie1[id].hp = packet->hp;
			b_zombie1[id]._animation = ZombieAnimationState::ATTACKED;
			//b_zombie1[id].hp_lock.unlock();
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			//b_zombie2[id].hp_lock.lock();
			b_zombie2[id].hp = packet->hp;
			b_zombie2[id]._animation = ZombieAnimationState::ATTACKED;
			//b_zombie2[id].hp_lock.unlock();
			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			//b_zombie3[id].hp_lock.lock();
			b_zombie3[id].hp = packet->hp;
			b_zombie3[id]._animation = ZombieAnimationState::ATTACKED;
			//b_zombie3[id].hp_lock.unlock();
			break;
		}
		}

		break;
	}
	case (int)MsgType::SC_WIN_STATE:
	{
		break;
	}
	case (int)MsgType::SC_ZOMBIE_ATTACK:
	{
		sc_zombie_attack_packet* packet = reinterpret_cast<sc_zombie_attack_packet*>(ptr);

		switch (packet->m_type)
		{
		case MapType::FIRST_PATH:
		{
			cout << (int)packet->id << "가 공격 했습니다\n";
			r_zombie1[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::SECOND_PATH:
		{
			cout << (int)packet->id << "가 공격 했습니다\n";
			r_zombie2[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::FINAL_PATH:
		{
			cout << (int)packet->id << "가 공격 했습니다\n";
			r_zombie3[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			cout << (int)packet->id << "가 공격 했습니다\n";
			b_zombie1[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			cout << (int)packet->id << "가 공격 했습니다\n";
			b_zombie2[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			cout << (int)packet->id << "가 공격 했습니다\n";
			b_zombie3[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		}
		break;
	}
	case (int)MsgType::SC_ZOMBIE_MAD:
	{
		break;
	}
	case (int)MsgType::SC_ZOMBIE_MOVE:
	{
		sc_zombie_move_packet* packet = reinterpret_cast<sc_zombie_move_packet*>(ptr);

		int id = packet->id;

		switch (packet->map_type)
		{
		case MapType::FIRST_PATH:
		{
			//move_lock.lock();
			r_zombie1[id].x = packet->x;
			r_zombie1[id].z = packet->z;
			r_zombie1[id].speed = packet->speed;
			r_zombie1[id].t_x = packet->t_x;
			r_zombie1[id].t_z = packet->t_z;
			r_zombie1[id].dir = packet->dir;
			r_zombie1[id].arrive = false;
			//r_zombie1[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			//cout << id << "의 좌표 x : " << packet->x << ", z : " << packet->z << "\n";
			break;
		}
		case MapType::SECOND_PATH:
		{
			//move_lock.lock();
			r_zombie2[id].x = packet->x;
			r_zombie2[id].z = packet->z;
			r_zombie2[id].speed = packet->speed;
			r_zombie2[id].t_x = packet->t_x;
			r_zombie2[id].t_z = packet->t_z;
			r_zombie2[id].dir = packet->dir;
			r_zombie2[id].arrive = false;
			//r_zombie2[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();
			//cout << id << "의 좌표 x : " << packet->x << ", z : " << packet->z << "\n";

			break;
		}
		case MapType::FINAL_PATH:
		{
			//move_lock.lock();
			r_zombie3[id].x = packet->x;
			r_zombie3[id].z = packet->z;
			r_zombie3[id].speed = packet->speed;
			r_zombie3[id].t_x = packet->t_x;
			r_zombie3[id].t_z = packet->t_z;
			r_zombie3[id].dir = packet->dir;
			r_zombie3[id].arrive = false;
			//r_zombie3[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			//move_lock.lock();
			b_zombie1[id].x = packet->x;
			b_zombie1[id].z = packet->z;
			b_zombie1[id].speed = packet->speed;
			b_zombie1[id].t_x = packet->t_x;
			b_zombie1[id].t_z = packet->t_z;
			b_zombie1[id].dir = packet->dir;
			b_zombie1[id].arrive = false;
		//	b_zombie1[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			//cout << id << "의 좌표 x : " << packet->x << ", z : " << packet->z << "\n";

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			//move_lock.lock();
			b_zombie2[id].x = packet->x;
			b_zombie2[id].z = packet->z;
			b_zombie2[id].speed = packet->speed;
			b_zombie2[id].t_x = packet->t_x;
			b_zombie2[id].t_z = packet->t_z;
			b_zombie2[id].dir = packet->dir;
			b_zombie2[id].arrive = false;
			//b_zombie2[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			//move_lock.lock();
			b_zombie3[id].x = packet->x;
			b_zombie3[id].z = packet->z;
			b_zombie3[id].speed = packet->speed;
			b_zombie3[id].t_x = packet->t_x;
			b_zombie3[id].t_z = packet->t_z;
			b_zombie3[id].dir = packet->dir;
			b_zombie3[id].arrive = false;
			//b_zombie3[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			break;
		}
		}

		break;
	}
	case (int)MsgType::SC_ZOMBIE_SPAWN:
	{
		sc_zombie_spawn_packet* packet = reinterpret_cast<sc_zombie_spawn_packet*>(ptr);

		map_type = packet->map_type;

		switch (map_type)
		{
		case MapType::FIRST_PATH:
		{
			int id = packet->id;

			r_zombie1[id].x = packet->x;
			r_zombie1[id].z = packet->z;
			r_zombie1[id].hp = packet->hp;
			r_zombie1[id]._id = id;
			r_zombie1[id]._state = packet->state;
			r_zombie1[id]._type = packet->zomtype;
			r_zombie1[id]._animation = ZombieAnimationState::SPAWN;
			r_zombie1[id].angle = packet->angle;
			//CAnimationObjectShader::GetInstance()->AddZombie(&r_zombie1[id]);
			//cout << "r_zombie1 [ " << id << "] 의 좌표 x : " << packet->x << ", z : " << packet->z << ", 체력 hp : " << packet->hp << "\n";
			//cout << "비교하기 위한 [ " << id << "]의 좌표 x : " << r_zombie1[id].x << ", z : " << r_zombie1[id].z << ", hp : " << r_zombie1[id].hp << "\n";
			break;
		}
		case MapType::SECOND_PATH:
		{
			int id = packet->id;

			r_zombie2[id].x = packet->x;
			r_zombie2[id].z = packet->z;
			r_zombie2[id].hp = packet->hp;
			r_zombie2[id]._id = packet->id;
			r_zombie2[id]._state = packet->state;
			r_zombie2[id]._type = packet->zomtype;
			r_zombie2[id]._animation = ZombieAnimationState::SPAWN;
			r_zombie2[id].angle = packet->angle;

			//cout << "r_zombie2 [ " << id << "] 의 좌표 x : " << packet->x << ", z : " << packet->z << ", 체력 hp : " << packet->hp << "\n";
			//cout << "비교하기 위한 [ " << id << "]의 좌표 x : " << r_zombie2[id].x << ", z : " << r_zombie2[id].z << ", hp : " << r_zombie2[id].hp << "\n";
			break;
		}
		case MapType::FINAL_PATH:
		{
			int id = packet->id;

			r_zombie3[id].x = packet->x;
			r_zombie3[id].z = packet->z;
			r_zombie3[id].hp = packet->hp;
			r_zombie3[id]._id = packet->id;
			r_zombie3[id]._state = packet->state;
			r_zombie3[id]._type = packet->zomtype;
			r_zombie3[id]._animation = ZombieAnimationState::SPAWN;
			r_zombie3[id].angle = packet->angle;

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			int id = packet->id;

			b_zombie1[id].x = packet->x;
			b_zombie1[id].z = packet->z;
			b_zombie1[id].hp = packet->hp;
			b_zombie1[id]._id = packet->id;
			b_zombie1[id]._state = packet->state;
			b_zombie1[id]._type = packet->zomtype;
			b_zombie1[id]._animation = ZombieAnimationState::SPAWN;
			b_zombie1[id].angle = packet->angle;

			//cout << "b_zombie1 [" << id << "] 의 좌표 x : " << packet->x << ", z : " << packet->z << ", 체력 hp : " << packet->hp << "\n";
			//cout << "비교하기 위한 [ " << id << "]의 좌표 x : " << b_zombie1[id].x << ", z : " << b_zombie1[id].z << ", hp : " << b_zombie1[id].hp << "\n";

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			int id = packet->id;

			b_zombie2[id].x = packet->x;
			b_zombie2[id].z = packet->z;
			b_zombie2[id].hp = packet->hp;
			b_zombie2[id]._id = packet->id;
			b_zombie2[id]._state = packet->state;
			b_zombie2[id]._type = packet->zomtype;
			b_zombie2[id]._animation = ZombieAnimationState::SPAWN;
			b_zombie2[id].angle = packet->angle;

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			int id = packet->id;

			b_zombie3[id].x = packet->x;
			b_zombie3[id].z = packet->z;
			b_zombie3[id].hp = packet->hp;
			b_zombie3[id]._id = packet->id;
			b_zombie3[id]._state = packet->state;
			b_zombie3[id]._type = packet->zomtype;
			b_zombie3[id]._animation = ZombieAnimationState::SPAWN;
			b_zombie3[id].angle = packet->angle;

			break;
		}
		}

		break;
	}
	case (int)MsgType::SC_ZOMBIE_ARRIVE:
	{
		sc_zombie_arrive_packet* packet = reinterpret_cast<sc_zombie_arrive_packet*>(ptr);

		switch(packet->map_type)
		{
		case MapType::FIRST_PATH:
		{
			r_zombie1[packet->id].arrive = true;
			r_zombie1[packet->id].dir = packet->dir;

			break;
		}
		case MapType::SECOND_PATH:
		{
			r_zombie2[packet->id].arrive = true;
			r_zombie2[packet->id].dir = packet->dir;

			break;
		}
		case MapType::FINAL_PATH:
		{
			r_zombie3[packet->id].arrive = true;
			r_zombie3[packet->id].dir = packet->dir;

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			b_zombie1[packet->id].arrive = true;
			b_zombie1[packet->id].dir = packet->dir;

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			b_zombie2[packet->id].arrive = true;
			b_zombie2[packet->id].dir = packet->dir;

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			b_zombie3[packet->id].arrive = true;
			b_zombie3[packet->id].dir = packet->dir;

			break;
		}
		}

		break;
	}
	case (int)MsgType::SC_ZOMBIE_VIEWLIST_PUT:
	{
		sc_zombie_viewlist_packet* packet = reinterpret_cast<sc_zombie_viewlist_packet*>(ptr);
		
		int z_id = packet->z_id;

		switch (packet->m_type)
		{
		case MapType::FIRST_PATH:
		{
			r_zombie1[z_id].view = true;
			r_zombie1[z_id].x = packet->x;
			r_zombie1[z_id].z = packet->z;
			r_zombie1[z_id]._type = packet->z_type;
			switch (packet->animation)
			{
			case MsgType::SC_ZOMBIE_MOVE:
			{
				r_zombie1[z_id]._animation = ZombieAnimationState::WALK;
				break;
			}
			case MsgType::SC_ZOMBIE_SPAWN:
			{
				r_zombie1[z_id]._animation = ZombieAnimationState::SPAWN;
				break;
			}
			default:
			{
				r_zombie1[z_id]._animation = ZombieAnimationState::IDLE;
			}
			}

			break;
		}
		case MapType::SECOND_PATH:
		{
			r_zombie2[z_id].view = true;
			r_zombie2[z_id].x = packet->x;
			r_zombie2[z_id].z = packet->z;
			r_zombie2[z_id]._type = packet->z_type;
			switch (packet->animation)
			{
			case MsgType::SC_ZOMBIE_MOVE:
			{
				r_zombie2[z_id]._animation = ZombieAnimationState::WALK;
				break;
			}
			case MsgType::SC_ZOMBIE_SPAWN:
			{
				r_zombie2[z_id]._animation = ZombieAnimationState::SPAWN;
				break;
			}
			default:
			{
				r_zombie2[z_id]._animation = ZombieAnimationState::IDLE;
			}
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			r_zombie3[z_id].view = true;
			r_zombie1[z_id].x = packet->x;
			r_zombie1[z_id].z = packet->z;
			r_zombie1[z_id]._type = packet->z_type;
			switch (packet->animation)
			{
			case MsgType::SC_ZOMBIE_MOVE:
			{
				r_zombie3[z_id]._animation = ZombieAnimationState::WALK;
				break;
			}
			case MsgType::SC_ZOMBIE_SPAWN:
			{
				r_zombie3[z_id]._animation = ZombieAnimationState::SPAWN;
				break;
			}
			default:
			{
				r_zombie3[z_id]._animation = ZombieAnimationState::IDLE;
			}
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			b_zombie1[z_id].view = true;
			b_zombie1[z_id].x = packet->x;
			b_zombie1[z_id].z = packet->z;
			b_zombie1[z_id]._type = packet->z_type;
			switch (packet->animation)
			{
			case MsgType::SC_ZOMBIE_MOVE:
			{
				b_zombie1[z_id]._animation = ZombieAnimationState::WALK;
				break;
			}
			case MsgType::SC_ZOMBIE_SPAWN:
			{
				b_zombie1[z_id]._animation = ZombieAnimationState::SPAWN;
				break;
			}
			default:
			{
				b_zombie1[z_id]._animation = ZombieAnimationState::IDLE;
			}
			}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			b_zombie2[z_id].view = true;
			b_zombie2[z_id].x = packet->x;
			b_zombie2[z_id].z = packet->z;
			b_zombie2[z_id]._type = packet->z_type;
			switch (packet->animation)
			{
			case MsgType::SC_ZOMBIE_MOVE:
			{
				b_zombie2[z_id]._animation = ZombieAnimationState::WALK;
				break;
			}
			case MsgType::SC_ZOMBIE_SPAWN:
			{
				b_zombie2[z_id]._animation = ZombieAnimationState::SPAWN;
				break;
			}
			default:
			{
				b_zombie2[z_id]._animation = ZombieAnimationState::IDLE;
			}
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			b_zombie3[z_id].view = true;
			b_zombie3[z_id].x = packet->x;
			b_zombie3[z_id].z = packet->z;
			b_zombie3[z_id]._type = packet->z_type;
			switch (packet->animation)
			{
			case MsgType::SC_ZOMBIE_MOVE:
			{
				b_zombie3[z_id]._animation = ZombieAnimationState::WALK;
				break;
			}
			case MsgType::SC_ZOMBIE_SPAWN:
			{
				b_zombie3[z_id]._animation = ZombieAnimationState::SPAWN;
				break;
			}
			default:
			{
				b_zombie3[z_id]._animation = ZombieAnimationState::IDLE;
			}
			}

			break;
		}
		}

		break;
	}
	case (int)MsgType::SC_ZOMBIE_VIEWLIST_REMOVE:
	{
		sc_zombie_viewlist_packet* packet = reinterpret_cast<sc_zombie_viewlist_packet*>(ptr);

		switch (packet->m_type)
		{
		case MapType::FIRST_PATH:
		{
			r_zombie1[packet->z_id].view = false;

			break;
		}
		case MapType::SECOND_PATH:
		{
			r_zombie2[packet->z_id].view = false;

			break;
		}
		case MapType::FINAL_PATH:
		{
			r_zombie3[packet->z_id].view = false;

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			b_zombie1[packet->z_id].view = false;

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			b_zombie2[packet->z_id].view = false;

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			b_zombie3[packet->z_id].view = false;

			break;
		}
		}

		break;
	}
	case (int)MsgType::SC_ZOMBIE_DEAD:
	{
		sc_zombie_dead_packet* packet = reinterpret_cast<sc_zombie_dead_packet*>(ptr);

		int id = packet->id;

		switch (packet->m_type)
		{
		case MapType::FIRST_PATH:
		{
			cout << (int)id << "죽음 \n";
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			//r_zombie1[id].~Zombie();
			break;
		}
		case MapType::SECOND_PATH:
		{
			cout << (int)id << "죽음 \n";
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			//r_zombie2[id].~Zombie();
			break;
		}
		case MapType::FINAL_PATH:
		{
			cout << (int)id << "죽음 \n";
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			//r_zombie3[id].~Zombie();
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			cout << (int)id << "죽음 \n";
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			//b_zombie1[id].~Zombie();
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			cout << (int)id << "죽음 \n";
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			//b_zombie2[id].~Zombie();
			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			cout << (int)id << "죽음 \n";
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			//b_zombie3[id].~Zombie();
			break;
		}
		}
		break;
	}
	case (int)MsgType::SC_ZOMBIE_SEARCH:
	{
		sc_zombie_search_packet* packet = reinterpret_cast<sc_zombie_search_packet*>(ptr);

		switch (packet->m_type)
		{
		case MapType::FIRST_PATH:
		{
			cout << (int)packet->z_id << "가 플레이어 " << (int)packet->p_id << "를 발견하였습니다 \n";
			break;
		}
		case MapType::SECOND_PATH:
		{
			cout << (int)packet->z_id << "가 플레이어 " << (int)packet->p_id << "를 발견하였습니다 \n";
			break;
		}
		case MapType::FINAL_PATH:
		{
			cout << (int)packet->z_id << "가 플레이어 " << (int)packet->p_id << "를 발견하였습니다 \n";
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			cout << (int)packet->z_id << "가 플레이어 " << (int)packet->p_id << "를 발견하였습니다 \n";
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			cout << (int)packet->z_id << "가 플레이어 " << (int)packet->p_id << "를 발견하였습니다 \n";
			break;
		case MapType::CHECK_POINT_FINAL:
		{
			cout << (int)packet->z_id << "가 플레이어 " << (int)packet->p_id << "를 발견하였습니다 \n";
			break;
		}
		}
		}
		break;
	}
	case (int)MsgType::SC_ZOMBIE_REMAIN:
	{
		cout << "좀비가 아직 남아있으므로 죽이고 오세요 \n";

		break;
	}
	case (int)MsgType::SC_WAIT:
	{
		
		break;
	}
	case (int)MsgType::SC_GAME_ALL_DEAD_END:
	{
		cout << "모든 플레이어가 사망하여 게임이 끝났습니다 \n";

		break;
	}
	case (int)MsgType::SC_GAME_END:
	{
		cout << "게임에 클리어하였습니다! 고생하셨습니다 \n";

		break;
	}
	default:
	{
		cout << packet_type << "으로 잘못왔음 \n";
		break;
	}
	}
	
}

void Network::send_login_packet(char* str)
{
	cs_login_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_LOGIN_REQUEST;
	strcpy_s(packet.name, str);
	_socket.do_send(sizeof(packet), &packet);
}

void volatile Network::ProcessData(Exp_Over& exp_over, int& size)
{
	int remain_data = size + _prev_size;
	unsigned char* packet_start = exp_over._net_buf;
	atomic<int> packet_size = (int)((packet_start[1] * 256) + packet_start[0]);

	while (packet_size <= remain_data)
	{
		cout << packet_size << "\n";

		ProcessPacket(packet_start);
		remain_data -= packet_size;
		packet_start += packet_size;
		if (remain_data > 0)
		{
			int temp = (int)((packet_start[1] * 256) + packet_start[0]);

			packet_size = temp;
		}
		else { 
			break;
		}
		if (packet_size == 0)
		{
			remain_data = 0;
			break;
		}
	}

	if (remain_data > 0)
	{
		_prev_size = remain_data;
		memcpy(&exp_over._net_buf, packet_start, remain_data);
	}
}

void Network::send_player_select_packet(PlayerType type)
{
	cs_select_packet packet;
	packet.playertype = type;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_PLAYER_SELECT;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::send_player_move_packet(float t_x, float t_z, float speed, float x, float z)
{
	cs_move_packet packet;
	packet.t_x = t_x;
	packet.t_z = t_z;
	packet.speed = speed;
	packet.x = x;
	packet.z = z;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_PLAYER_MOVE;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::Send_chat_packet(char* msg)
{
	cs_chat_packet packet;
	packet.type = MsgType::CS_PLAYER_CHAT;
	memcpy(packet.message, msg, MAX_CHAT_SIZE);
	packet.size = sizeof(packet);

	_socket.do_send(sizeof(packet), &packet);
}

void Network::PlayerMove(int p_id)
{
	// 0.1초마다 플레이어 좌표, 방향, 속도 보내기
	send_player_move_packet(g_client[p_id].t_x, g_client[p_id].t_z, g_client[p_id].speed, g_client[p_id].x, g_client[p_id].z);

	AddTimer(p_id, EVENT_TYPE::PLAYER_MOVE, 100);
}

void Network::Work()
{
	// 0. 클라이언트 초기화
	Initialize();

	// 1. 로그인 요청
	char name[MAX_NAME_SIZE];

	while (true) {
		cout << "login : ";
		cin >> name;
		send_login_packet(name);

		Sleep(10);

		_socket.do_recv(_prev_size);
		unsigned char* login_buf = _socket._recv_over._net_buf;
		int remain_data = 200;
		unsigned char* temp_buf = login_buf;
		int temp_size = (int)temp_buf[1] * 256 + (int)temp_buf[0];
		while (true)
		{
			if (temp_size != 0)
			{
				ProcessPacket(temp_buf);
				remain_data -= temp_size;
				temp_buf += temp_size;
				if (remain_data > 0)
				{
					temp_size = (int)temp_buf[1] * 256 + (int)temp_buf[0];
				}
				else
					break;
			}
			else
				break;
		}

		if (login_complete)
			break;
	}

	int select;
	PlayerType type = PlayerType::NONE;
	select_complete = false;
	while (1) {
		HANDLE t_iocp = _socket.ReturnHandle();
		DWORD num_byte;
		LONG64 iocp_key;
		WSAOVERLAPPED* p_over;

		BOOL ret = GetQueuedCompletionStatus(t_iocp, &num_byte, (PULONG_PTR)&iocp_key, &p_over, INFINITE);
		total_time = chrono::system_clock::now() + 1000ms;
		int c_id = static_cast<int>(iocp_key);
		Exp_Over* exp_over = reinterpret_cast<Exp_Over*>(p_over);
		if (ret == SOCKET_ERROR)
			throw Exception("Queue Error");

		switch (exp_over->_IOType)
		{
		case IOType::RECV:
			//ProcessData((unsigned char*)_socket._recv_over._net_buf, (_socket._recv_over._net_buf[1] * 256) + _socket._recv_over._net_buf[0]);
			ProcessData(*exp_over, (int&)num_byte);

			_socket.do_recv(_prev_size);
			break;
		case IOType::SEND:
			if (num_byte != exp_over->_wsa_buf.len) {
				cout << "펑 \n";
				exit(0);
			}
			delete exp_over;
			exp_over = nullptr;
			break;
		case IOType::PLAYER_MOVE:
		{
			PlayerMove(c_id);

			delete exp_over;
			exp_over = nullptr;
		}
			break;
		default:
			cout << "이건 아무것도 아님 \n";

		}

		// 2. 플레이어 캐릭터 선택
		//select_lock.lock();
		if (select_complete == false)
		{
			cout << "플레이어 선택 \n";
			cout << "1. 지휘관, 2. 엔지니어 3. 용병 \n";
			cin >> select;
			select_complete = true;
			if (select == 1)
				type = PlayerType::COMMANDER;
			else if (select == 2)
				type = PlayerType::ENGINEER;
			else if (select == 3)
				type = PlayerType::MERCENARY;

			send_player_select_packet(type);
			this_thread::sleep_for(10ms);

			Send_request_packet(MsgType::CS_BARRICADE_REQUEST);
			this_thread::sleep_for(10ms);
			//select_lock.unlock();
			continue;
		}
		//select_lock.unlock();
	}
	
}

void Network::Do_Timer()
{
	while (true)
	{
		this_thread::sleep_for(1ms);
		
		if (total_time < chrono::system_clock::now() && select_complete == true && g_client[my_id]._state == ClientState::INGAME)
		{
			if (barricade_req == false)
			{
				Send_request_packet(MsgType::CS_BARRICADE_REQUEST);
				this_thread::sleep_for(10ms);
			}
			else if (barricade_req == true && game_start == false)
			{
				Send_request_packet(MsgType::CS_GAME_START_REQUEST);
				this_thread::sleep_for(10ms);
			}

			Send_request_packet(MsgType::CS_SERVER_REQUEST);

			total_time = chrono::system_clock::now() + 1000ms;
		}

		if (key_down_time < chrono::system_clock::now() && key_down_state == true)
		{
			key_down_state = false;
			key_down_time = chrono::system_clock::now() + 200ms;
		}

		if (mouse_time < chrono::system_clock::now() && mouse_state == true)
		{
			mouse_state = false;
			mouse_time = chrono::system_clock::now() + 33ms;
		}

		if (attack_time < chrono::system_clock::now() && attack_state == true)
		{
			attack_state = false;
			attack_time = chrono::system_clock::now() + 250ms;
		}

		while (true)
		{
			timer_lock.lock();
			if (timer_queue.empty() == true)
			{
				timer_lock.unlock();
				break;
			}
			if (timer_queue.top().start_time > chrono::high_resolution_clock::now())
			{
				timer_lock.unlock();
				break;
			}

			Timer_Event te = timer_queue.top();
			timer_queue.pop();
			timer_lock.unlock();

			switch (te.ev)
			{
			case EVENT_TYPE::PLAYER_MOVE:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::PLAYER_MOVE;
				HANDLE t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.p_id, &over->_wsa_over);
				break;
			}
			}
		}
	}
}

void Network::ZombieAngle(Zombie& zombie, float time_elapsed)
{
	float angle = zombie.angle;

	switch (zombie.dir)
	{
	case Direction::NONE:
	{
		
	}
		break;
	case Direction::UP:
	{
		if (angle < -2.0f && angle > -180.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (angle > 2.0f && angle < 180.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else
			zombie.angle = 0.0f;

		break;
	}
	case Direction::UP_RIGHT:
	{
		if (0.0f <= angle && angle < 43.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-135.0f <= angle && angle < 0.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-180.0f <= angle && angle < -135.0f)
		{
			angle -= 180.0f * time_elapsed;
			if (angle < -180.0f)
				angle += 360.0f;
			zombie.angle = angle;
		}
		else if (47.0f <= angle && angle <= 180.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else
		{
			zombie.angle = 45.0f;
		}

		break;
	}
	case Direction::UP_LEFT:
	{
		if (-43.0f <= angle && angle <= 0.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (0.0f <= angle && angle <= 135.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-180.0f <= angle && angle <= -47.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (135.0f <= angle && angle <= 180.0f)
		{
			angle += 180.0f * time_elapsed;
			if (angle > 180.0f)
				angle -= 360.0f;
			zombie.angle = angle;
		}
		else
		{
			zombie.angle = -45.0f;
		}

		break;
	}
	case Direction::RIGHT:
	{
		if (-90.0f <= angle && angle <= 88.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-180.0f <= angle && angle < -90.0f)
		{
			angle -= 180.0f * time_elapsed;
			if (angle < -180.0f)
				angle += 360.0f;
			zombie.angle = angle;
		}
		else if (92.0f <= angle && angle <= 180.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else
			zombie.angle = 90.0f;

		break;
	}
	case Direction::LEFT:
	{
		if (-88.0f <= angle && angle <= 90.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-180.0f <= angle && angle <= -92.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (90.0f < angle && angle <= 180.0f)
		{
			angle -= 180.0f * time_elapsed;
			if (angle < -180.0f)
				angle += 360.0f;
			zombie.angle = angle;
		}
		else
			zombie.angle = -90.0f;

		break;
	}
	case Direction::DOWN:
	{
		if (0.0f <= angle && angle <= 178.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-178.0f <= angle && angle < 0.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else
			zombie.angle = 180.0f;
		break;
	}
	case Direction::DOWN_LEFT:
	{
		if (-133.0f <= angle && angle <= 45.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (45.0f < angle && angle <= 180.0f)
		{
			angle += 180.0f * time_elapsed;
			if (angle > 180.0f)
				angle -= 360.0f;
			zombie.angle = angle;
		}
		else if (-180.0f <= angle && angle <= -137.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else
			zombie.angle = -135.0f;

		break;
	}
	case Direction::DOWN_RIGHT:
	{
		if (-45.0f <= angle && angle <= 133.0f)
		{
			angle += 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else if (-180.0f <= angle && angle < -45.0f)
		{
			angle -= 180.0f * time_elapsed;
			if (angle < -180.0f)
				angle += 360.0f;
			zombie.angle = angle;
		}
		else if (137.0f <= angle && angle <= 180.0f)
		{
			angle -= 180.0f * time_elapsed;
			zombie.angle = angle;
		}
		else
			zombie.angle = 135.0f;

		break;
	}
	}
}

void Network::ZombieMove(Zombie& zombie, float time_elapsed)
{
	float x, z;
	x = zombie.x + zombie.speed * zombie.t_x * time_elapsed;
	z = zombie.z + zombie.speed * zombie.t_z * time_elapsed;

	int row = z;
	int col = x;

	float row_result = z - row;
	float col_result = x - col;

	if (row_result >= 0.5f) row += 1;
	if (col_result >= 0.5f) col += 1;

	if (!zombie.IsCollied(row, col, map))
	{
		zombie.x = x;
		zombie.z = z;
	}
}
//float test_time = 0.0f;

void Network::Update(float time_elapsed)
{
	for (auto & player : g_client) {
		
		if (player._state != ClientState::INGAME) continue;

		//if(player._id != my_id)
		//	if(player._animation != ClientAnimationState::WALK)
		//		player._animation = ClientAnimationState::WALK;
		player.move(time_elapsed);
	}
	/*test_time += time_elapsed;
	if (test_time > 3.0f) {
		test_time = 0.0f;
		switch (r_zombie1[0]._animation) {
		case ZombieAnimationState::ATTACK:
			r_zombie1[0]._animation = ZombieAnimationState::SPAWN;
			cout << "state: SPAWN" << endl;
			break;
		case ZombieAnimationState::SPAWN:
			r_zombie1[0]._animation = ZombieAnimationState::DEAD;
			cout << "state: DEAD" << endl;
			break;
		case ZombieAnimationState::DEAD:
			r_zombie1[0]._animation = ZombieAnimationState::ATTACKED;
			cout << "state: ATTACKED" << endl;
			break;
		case ZombieAnimationState::ATTACKED:
			r_zombie1[0]._animation = ZombieAnimationState::IDLE;
			cout << "state: IDLE" << endl;
			break;
		case ZombieAnimationState::IDLE:
			r_zombie1[0]._animation = ZombieAnimationState::WALK;
			cout << "state: WALK" << endl;
			break;
		case ZombieAnimationState::WALK:
			r_zombie1[0]._animation = ZombieAnimationState::SPAWN;
			cout << "state: SPAWN" << endl;
			break;
		default:
			r_zombie1[0]._animation = ZombieAnimationState::SPAWN;
			cout << "state: SPAWN" << endl;
			break;
		}
		
	}*/
	
	switch (map_type)
	{
	case MapType::FIRST_PATH:
	{
		for (auto& zombie : r_zombie1)
		{
			if (zombie._state != ZombieState::SPAWN) continue;

			ZombieAngle(zombie, time_elapsed);

			if (zombie.arrive) continue;

			ZombieMove(zombie, time_elapsed);
		}
		break;
	}
	case MapType::SECOND_PATH:
	{
		for (auto& zombie : r_zombie2)
		{
			if (zombie._state != ZombieState::SPAWN) continue;

			ZombieAngle(zombie, time_elapsed);

			if (zombie.arrive) continue;

			ZombieMove(zombie, time_elapsed);
		}

		break;
	}
	case MapType::FINAL_PATH:
	{
		for (auto& zombie : r_zombie3)
		{
			if (zombie._state != ZombieState::SPAWN) continue;

			ZombieAngle(zombie, time_elapsed);

			if (zombie.arrive) continue;

			ZombieMove(zombie, time_elapsed);
		}

		break;
	}
	case MapType::CHECK_POINT_ONE:
	{
		for (auto& zombie : b_zombie1)
		{
			if (zombie._state != ZombieState::SPAWN) continue;

			ZombieAngle(zombie, time_elapsed);

			if (zombie.arrive) continue;

			ZombieMove(zombie, time_elapsed);
		}

		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		for (auto& zombie : b_zombie2)
		{
			if (zombie._state != ZombieState::SPAWN) continue;

			ZombieAngle(zombie, time_elapsed);

			if (zombie.arrive) continue;

			ZombieMove(zombie, time_elapsed);
		}

		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		for (auto& zombie : b_zombie3)
		{
			if (zombie._state != ZombieState::SPAWN) continue;

			ZombieAngle(zombie, time_elapsed);

			if (zombie.arrive) continue;

			ZombieMove(zombie, time_elapsed);
		}

		break;
	}
	}
}