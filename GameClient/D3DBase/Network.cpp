#include "Network.h"
#include "AnimationShader.h"
#include "SoundSystem.h"
#include "CStaticObjectShader.h"
#include "GameFramework.h"

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
BarricadePos Network::skill_barricade[3];

atomic_int Network::fps_rate;
atomic_int Network::remain_zombie;

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
	CSoundSystem::GetInstance()->Play(L"test bgm");
	ReadMapFile();

	map_type = MapType::SPAWN;

	select_complete = false;
	login_complete = false;
	game_start = false;
	barricade_req = false;
	key_down_state = false;
	total_time = chrono::system_clock::now();
	key_down_time = chrono::system_clock::now();
	remain_zombie = 0;

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
	if (pos.angle == ANGLE::ZERO)
	{
		temp.angle = 90;
	}
	else if (pos.angle == ANGLE::FIFTEEN)
	{
		temp.angle = 75;
	}
	else if (pos.angle == ANGLE::THIRTY)
	{
		temp.angle = 60;
	}
	else if (pos.angle == ANGLE::FORTY_FIVE)
	{
		temp.angle = 45;
	}
	else if (pos.angle == ANGLE::SIXTY)
	{
		temp.angle = 30;
	}
	else if (pos.angle == ANGLE::SEVENTY_FIVE)
	{
		temp.angle = 15;
	}
	else if (pos.angle == ANGLE::NINETY)
	{
		temp.angle = 0;
	}
	else if (pos.angle == ANGLE::ONE_HUNDRED_FIVE)
	{
		temp.angle = 345;
	}
	else if (pos.angle == ANGLE::ONE_HUNDRED_TWENTY)
	{
		temp.angle = 330;
	}
	else if (pos.angle == ANGLE::ONE_HUNDRED_THIRTY_FIVE)
	{
		temp.angle = 315;
	}
	else if (pos.angle == ANGLE::ONE_HUNDERED_FIFTY)
	{
		temp.angle = 300;
	}
	else if (pos.angle == ANGLE::ONE_HUNDRED_SIXTY_FIVE)
	{
		temp.angle = 285;
	}
	else if (pos.angle == ANGLE::ONE_HUNDRED_EIGHTY)
	{
		temp.angle = 270;
	}
	else if (pos.angle == ANGLE::ONE_HUNDRED_NINETY_FIVE)
	{
		temp.angle = 255;
	}
	else if (pos.angle == ANGLE::TWO_HUNDRED_TEN)
	{
		temp.angle = 240;
	}
	else if (pos.angle == ANGLE::TWO_HUNDRED_TWENTY_FIVE)
	{
		temp.angle = 225;
	}
	else if (pos.angle == ANGLE::TWO_HUNDRED_FORTY)
	{
		temp.angle = 210;
	}
	else if (pos.angle == ANGLE::TWO_HUNDRED_FIFTY_FIVE)
	{
		temp.angle = 195;
	}
	else if (pos.angle == ANGLE::TWO_HUNDRED_SEVENTY)
	{
		temp.angle = 180;
	}
	else if (pos.angle == ANGLE::TWO_HUNDRED_EIGHTY_FIVE)
	{
		temp.angle = 165;
	}
	else if (pos.angle == ANGLE::THREE_HUNDRED)
	{
		temp.angle = 150;
	}
	else if (pos.angle == ANGLE::THREE_HUNDRED_FIFTEEN)
	{
		temp.angle = 135;
	}
	else if (pos.angle == ANGLE::THREE_HUNDRED_THIRD)
	{
		temp.angle = 120;
	}
	else if (pos.angle == ANGLE::THREE_HUNDRED_FORTY_FIVE)
	{
		temp.angle = 105;
	}
	temp.b_type = pos.b_type;
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

void Network::Send_commander_special_req_packet(int c_id)
{
	cs_special_req_packet packet;
	packet.size = sizeof(packet);
	packet.id = c_id;
	packet.type = MsgType::CS_SPECIAL_SKILL_REQUEST;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::Send_commander_special_change_packet(int c_id)
{
	cs_special_req_packet packet;
	packet.size = sizeof(packet);
	packet.id = c_id;
	packet.type = MsgType::CS_SPECIAL_SKILL_CHANGE;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::ChangeWall(iPos pos, ANGLE angle)
{
	if (angle == ANGLE::ZERO || angle == ANGLE::ONE_HUNDRED_EIGHTY)													// 0도 , 180도
	{
		for (int j = pos.x - 1; j <= pos.x + 1; ++j)
		{
			for (int i = pos.z - 2; i <= pos.z + 2; ++i)
			{
				map[i][j] = (char)MazeWall::BARRICADE;
			}
		}
	}
	else if (angle == ANGLE::FIFTEEN || angle == ANGLE::ONE_HUNDRED_NINETY_FIVE)								// 15도, 195도
	{
		for (int j = pos.x - 1; j <= pos.x + 1; ++j)
		{
			for (int i = pos.z - 2; i <= pos.z + 2; ++i)
			{
				map[i][j] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 3][pos.x - 1] = (char)MazeWall::BARRICADE;
		map[pos.z - 3][pos.x] = (char)MazeWall::BARRICADE;

		map[pos.z - 2][pos.x + 2] = (char)MazeWall::BARRICADE;
		map[pos.z - 1][pos.x + 2] = (char)MazeWall::BARRICADE;
		map[pos.z][pos.x + 2] = (char)MazeWall::BARRICADE;

		map[pos.z][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z + 1][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z + 2][pos.x - 2] = (char)MazeWall::BARRICADE;

		map[pos.z + 3][pos.x] = (char)MazeWall::BARRICADE;
		map[pos.z + 3][pos.x + 1] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::THIRTY || angle == ANGLE::TWO_HUNDRED_TEN)												// 30도, 210도
	{
		for (int j = pos.x - 1; j <= pos.x + 1; ++j)
		{
			for (int i = pos.z - 2; i <= pos.z + 2; ++i)
			{
				map[i][j] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 3][pos.x] = (char)MazeWall::BARRICADE;

		map[pos.z - 2][pos.x + 2] = (char)MazeWall::BARRICADE;
		map[pos.z - 1][pos.x + 2] = (char)MazeWall::BARRICADE;
		map[pos.z][pos.x + 2] = (char)MazeWall::BARRICADE;

		map[pos.z][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z + 1][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z + 2][pos.x - 2] = (char)MazeWall::BARRICADE;

		map[pos.z + 3][pos.x] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::FORTY_FIVE || angle == ANGLE::TWO_HUNDRED_TWENTY_FIVE)						// 45도, 225도
	{
		for (int z = pos.z; z <= pos.z + 2; ++z)
		{
			for (int x = pos.x - 2; x <= pos.x; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int z = pos.z - 2; z <= pos.z; ++z)
		{
			for (int x = pos.x; x <= pos.x + 2; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 3][pos.x + 1] = (char)MazeWall::BARRICADE;

		map[pos.z - 1][pos.x - 1] = (char)MazeWall::BARRICADE;
		map[pos.z - 1][pos.x + 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x + 1] = (char)MazeWall::BARRICADE;
		map[pos.z + 1][pos.x - 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 3][pos.x - 1] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::SIXTY || angle == ANGLE::TWO_HUNDRED_FORTY)											// 60도, 240도
	{
		for (int z = pos.z; z <= pos.z + 2; ++z)
		{
			for (int x = pos.x - 2; x <= pos.x; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int z = pos.z - 2; z <= pos.z; ++z)
		{
			for (int x = pos.x; x <= pos.x + 2; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 1][pos.x - 1] = (char)MazeWall::BARRICADE;

		map[pos.z][pos.x - 3] = (char)MazeWall::BARRICADE;
		map[pos.z][pos.x + 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x + 1] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::SEVENTY_FIVE || angle == ANGLE::TWO_HUNDRED_FIFTY_FIVE)						// 75도, 255도
	{
		for (int j = pos.z - 1; j <= pos.z + 1; ++j)
		{
			for (int i = pos.x - 2; i <= pos.x + 2; ++i)
			{
				map[j][i] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 2][pos.x + 1] = (char)MazeWall::BARRICADE;
		map[pos.z - 2][pos.x + 2] = (char)MazeWall::BARRICADE;

		map[pos.z - 1][pos.x - 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x + 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 2][pos.x - 1] = (char)MazeWall::BARRICADE;
		map[pos.z + 2][pos.x - 2] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::NINETY || angle == ANGLE::TWO_HUNDRED_SEVENTY)										// 90도, 270도
	{
		for (int j = pos.z - 1; j <= pos.z + 1; ++j)
		{
			for (int i = pos.x - 2; i <= pos.x + 2; ++i)
			{
				map[j][i] = (char)MazeWall::BARRICADE;
			}
		}
	}
	else if (angle == ANGLE::ONE_HUNDRED_FIVE || angle == ANGLE::TWO_HUNDRED_EIGHTY_FIVE)			// 105도, 285도
	{
		for (int j = pos.z - 1; j <= pos.z + 1; ++j)
		{
			for (int i = pos.x - 2; i <= pos.x + 2; ++i)
			{
				map[j][i] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 2][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z - 2][pos.x - 1] = (char)MazeWall::BARRICADE;

		map[pos.z - 1][pos.x + 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x - 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 2][pos.x + 1] = (char)MazeWall::BARRICADE;
		map[pos.z + 2][pos.x + 2] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::ONE_HUNDRED_TWENTY || angle == ANGLE::THREE_HUNDRED)						// 120도, 300도
	{
		for (int z = pos.z - 2; z <= pos.z; ++z)
		{
			for (int x = pos.x - 2; x <= pos.x; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int z = pos.z; z <= pos.z + 2; ++z)
		{
			for (int x = pos.x; x <= pos.x + 2; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 1][pos.x + 1] = (char)MazeWall::BARRICADE;

		map[pos.z][pos.x - 3] = (char)MazeWall::BARRICADE;
		map[pos.z][pos.x + 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x - 1] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::ONE_HUNDRED_THIRTY_FIVE || angle == ANGLE::THREE_HUNDRED_FIFTEEN)	// 135도, 315도
	{
		for (int z = pos.z - 2; z <= pos.z; ++z)
		{
			for (int x = pos.x - 2; x <= pos.x; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int z = pos.z; z <= pos.z + 2; ++z)
		{
			for (int x = pos.x; x <= pos.x + 2; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 3][pos.x - 1] = (char)MazeWall::BARRICADE;

		map[pos.z - 1][pos.x - 3] = (char)MazeWall::BARRICADE;
		map[pos.z - 1][pos.x + 1] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x - 1] = (char)MazeWall::BARRICADE;
		map[pos.z + 1][pos.x + 3] = (char)MazeWall::BARRICADE;

		map[pos.z + 3][pos.x + 1] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::ONE_HUNDERED_FIFTY || angle == ANGLE::THREE_HUNDRED_THIRD)				// 150도, 330도
	{
		for (int z = pos.z - 2; z <= pos.z; ++z)
		{
			for (int x = pos.x - 2; x <= pos.x; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int z = pos.z; z <= pos.z + 2; ++z)
		{
			for (int x = pos.x; x <= pos.x + 2; ++x)
			{
				map[z][x] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 3][pos.x] = (char)MazeWall::BARRICADE;

		map[pos.z - 1][pos.x + 1] = (char)MazeWall::BARRICADE;

		map[pos.z + 1][pos.x - 1] = (char)MazeWall::BARRICADE;

		map[pos.z + 3][pos.x] = (char)MazeWall::BARRICADE;
	}
	else if (angle == ANGLE::ONE_HUNDRED_SIXTY_FIVE || angle == ANGLE::THREE_HUNDRED_FORTY_FIVE)	// 165도, 345도
	{
		for (int j = pos.x - 1; j <= pos.x + 1; ++j)
		{
			for (int i = pos.z - 2; i <= pos.z + 2; ++i)
			{
				map[i][j] = (char)MazeWall::BARRICADE;
			}
		}

		map[pos.z - 3][pos.x] = (char)MazeWall::BARRICADE;
		map[pos.z - 3][pos.x + 1] = (char)MazeWall::BARRICADE;

		map[pos.z - 2][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z - 1][pos.x - 2] = (char)MazeWall::BARRICADE;
		map[pos.z][pos.x - 2] = (char)MazeWall::BARRICADE;

		map[pos.z][pos.x + 2] = (char)MazeWall::BARRICADE;
		map[pos.z + 1][pos.x + 2] = (char)MazeWall::BARRICADE;
		map[pos.z + 2][pos.x + 2] = (char)MazeWall::BARRICADE;

		map[pos.z + 3][pos.x - 1] = (char)MazeWall::BARRICADE;
		map[pos.z + 3][pos.x] = (char)MazeWall::BARRICADE;
	}
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
			other_client_id1 = id;
			g_client[other_client_id1]._state = ClientState::INGAME;

			g_client[other_client_id1]._animation = ClientAnimationState::IDLE;
			id_lock.unlock();

			break;
		}
		if (other_client_id2 == -1 && other_client_id1 != id)
		{
			id_lock.lock();
			g_client[id]._id = packet->id;
			other_client_id2 = id;
			g_client[other_client_id2]._state = ClientState::INGAME;

			g_client[other_client_id2]._animation = ClientAnimationState::IDLE;
			id_lock.unlock();

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
			ChangeWall(packet->one_base[i], packet->one_base[i].angle);
			
			i++;
		}

		i = 0;
		for (auto& bar : two_barricade)
		{
			bar = Change_Client_Pos(packet->two_base[i]);
			ChangeWall(packet->two_base[i], packet->two_base[i].angle);
			
			i++;
		}

		i = 0;
		for (auto& bar : three_barricade)
		{
			bar = Change_Client_Pos(packet->three_base[i]);
			ChangeWall(packet->three_base[i], packet->three_base[i].angle);
			
			i++;
		}

		i = 0;
		for (auto& bar : three_barricade2)
		{
			bar = Change_Client_Pos(packet->three_base2[i]);
			ChangeWall(packet->three_base2[i], packet->three_base2[i].angle);
			
			i++;
		}

		for (auto& bar : skill_barricade)
		{
			bar.x = 800;
			bar.z = 800;
		}

		Send_request_packet(MsgType::CS_GAME_START_REQUEST);
		barricade_req = true;

		break;
	}
	case (int)MsgType::SC_GAME_START:
	{
		cout << "게임 시작 \n";
		game_start = true;
		Send_request_packet(MsgType::CS_GAME_START);
		CSoundSystem::GetInstance()->StopBGM();
		CSoundSystem::GetInstance()->Play(L"in game bgm");
		AddTimer(my_id, EVENT_TYPE::PLAYER_MOVE, 100);
		//change to game scene
		auto framework = CGameFramework::GetInstance();
		auto sig = framework->GetCurruntScene()->GetGraphicsRootSignature();
		framework->ChangeScene(new CMainGameScene(sig));
		break;
	}
	case (int)MsgType::SC_GAME_START_FAIL:
	{
		break;
	}
	case (int)MsgType::SC_PLAYER_ATTACK:
	{
		CSoundSystem::GetInstance()->Play(L"gun fire");
		break;
	}
	case (int)MsgType::SC_PLAYER_KILL_NUMBER:
	{
		sc_player_zombie_klil_packet* packet = reinterpret_cast<sc_player_zombie_klil_packet*>(ptr);

		g_client[packet->id].zombie_kill_num = packet->zom_num;

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

		break;
	}
	case (int)MsgType::SC_PLAYER_RELOAD:
	{
		sc_player_reload_packet* packet = reinterpret_cast<sc_player_reload_packet*>(ptr);


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


		break;
	}
	case (int)MsgType::SC_PLAYER_INTERATION:
	{
		break;
	}
	case (int)MsgType::SC_DOOR_OPEN:
	{
		sc_door_open_packet* packet = reinterpret_cast<sc_door_open_packet*>(ptr);


		break;
	}
	case (int)MsgType::SC_PLAYER_NOT_ENGINEER:
	{

		break;
	}
	case (int)MsgType::SC_PLAYER_MOVE:
	{
		sc_player_move_packet* packet = reinterpret_cast<sc_player_move_packet*>(ptr);
		
		int id = packet->id;
		if (id != Network::my_id) {
			g_client[id].x = packet->x;
			g_client[id].z = packet->z;
			g_client[id].t_x = packet->t_x;
			g_client[id].t_z = packet->t_z;
			g_client[id].speed = packet->speed;
			//g_client[id].is_input = packet->in_input;
			//g_client[id].rotation_angle = packet->rotation_angle;
		}
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

		if (g_client[id]._type == PlayerType::COMMANDER)
		{
			g_client[id].special_skill = 1;
		}
		else if (g_client[id]._type == PlayerType::ENGINEER)
		{
			g_client[id].special_skill = 3;
		}
		else if (g_client[id]._type == PlayerType::MERCENARY)
		{
			g_client[id].special_skill = 3;
		}

		g_client[id].hp = packet->hp;
		g_client[id].maxhp = packet->maxhp;
		g_client[id].x = packet->x;
		g_client[id].z = packet->z;
		g_client[id].bullet = packet->bullet;
		//g_client[id].speed = packet->speed;
		g_client[id].max_speed = packet->speed;;

		break;
	}
	case (int)MsgType::SC_PLAYER_SELECT_FAIL:
	{
		sc_fail_packet* packet = reinterpret_cast<sc_fail_packet*>(ptr);

		if(g_client[packet->id]._type == PlayerType::NONE)
			select_complete = false;

		break;
	}
	case (int)MsgType::SC_PLAYER_SPECIAL:
	{
		break;
	}
	case (int)MsgType::SC_COMMANDER_SPECIAL:
	{
		sc_commander_special_packet* packet = reinterpret_cast<sc_commander_special_packet*>(ptr);

		if (packet->id == my_id)
		{
			cout << "부활하였습니다 \n";
		}
		else
		{
			if (g_client[packet->id]._type == PlayerType::ENGINEER)
			{
				cout << "엔지니어가 부활하였습니다. \n";
			}
			else
			{
				cout << "용병이 부활하였습니다. \n";
			}
		}

		g_client[packet->id]._state = ClientState::INGAME;
		g_client[packet->id].hp = packet->hp;
		g_client[packet->id].bullet = packet->bullet;
		g_client[packet->id].special_skill -= 1;

		break;
	}
	case (int)MsgType::SC_COMMANDER_SPECIAL_CHECK:
	{
		sc_player_co_special_check_packet* packet = reinterpret_cast<sc_player_co_special_check_packet*>(ptr);

		if (g_client[packet->id]._type == PlayerType::ENGINEER)
		{
			//cout << "엔지니어를 부활 시키겠습니까? 바꾸려면 C, 부활시키려면 V를 눌러주세요. \n";
		}
		else
		{
		//	cout << "용병을 부활 시키겠습니까? 바꾸려면 C, 부활시키려면 V를 눌러주세요. \n";
		}

		g_client[my_id].special_skill_key = true;
		g_client[my_id].special_id = packet->id;

		break;
	}
	case (int)MsgType::SC_ENGINEER_SPECIAL:
	{
		sc_engineer_barrigate_build_packet* packet = reinterpret_cast<sc_engineer_barrigate_build_packet*>(ptr);
		
		BarricadePos temp;
		temp.x = (packet->x - 550.0f) * (-100.0f);
		temp.z = (packet->z - 210.0f) * (-100.0f);
		temp.b_type = BarricadeType::BARRICADE;
		
		switch (packet->dir)
		{
		case Direction::UP:
		case Direction::DOWN:
		{
			temp.angle = 0.0f;
			for (int j = 0; j < 3; ++j)
			{
				for (int i = 0; i < 5; ++i)
				{
					map[packet->z - 1 + j][packet->x - 2 + i] = (char)MazeWall::BARRICADE;
				}
			}
			break;
		}
		case Direction::LEFT:
		case Direction::RIGHT:
		{
			temp.angle = 90.0f;
			for (int j = 0; j < 3; ++j)
			{
				for (int i = 0; i < 5; ++i)
				{
					map[packet->z - 2 + i][packet->x - 1 + j] = (char)MazeWall::BARRICADE;
				}
			}
			break;
		}
		case Direction::UP_LEFT:
		case Direction::DOWN_RIGHT:
		{
			temp.angle = 45.0f;
			for (int t_z = packet->z - 2; t_z <= packet->z; ++t_z)
			{
				for (int t_x = packet->x - 2; t_x <= packet->x; ++t_x)
				{
					map[t_z][t_x] = (char)MazeWall::BARRICADE;
				}
			}

			for (int t_z = packet->z; t_z <= packet->z + 2; ++t_z)
			{
				for (int t_x = packet->x; t_x <= packet->x + 2; ++t_x)
				{
					map[t_z][t_x] = (char)MazeWall::BARRICADE;
				}
			}

			map[packet->z - 3][packet->x - 1] = (char)MazeWall::BARRICADE;

			map[packet->z - 1][packet->x - 3] = (char)MazeWall::BARRICADE;
			map[packet->z - 1][packet->x + 1] = (char)MazeWall::BARRICADE;

			map[packet->z + 1][packet->x - 1] = (char)MazeWall::BARRICADE;
			map[packet->z + 1][packet->x + 3] = (char)MazeWall::BARRICADE;

			map[packet->z + 3][packet->x + 1] = (char)MazeWall::BARRICADE;

			break;
		}
		case Direction::UP_RIGHT:
		case Direction::DOWN_LEFT:
		{
			temp.angle = 135.0f;
			for (int t_z = packet->z; t_z <= packet->z + 2; ++t_z)
			{
				for (int t_x = packet->x - 2; t_x <= packet->x; ++t_x)
				{
					map[t_z][t_x] = (char)MazeWall::BARRICADE;
				}
			}

			for (int t_z = packet->z - 2; t_z <= packet->z; ++t_z)
			{
				for (int t_x = packet->x; t_x <= packet->x + 2; ++t_x)
				{
					map[t_z][t_x] = (char)MazeWall::BARRICADE;
				}
			}

			map[packet->z - 3][packet->x + 1] = (char)MazeWall::BARRICADE;

			map[packet->z - 1][packet->x - 1] = (char)MazeWall::BARRICADE;
			map[packet->z - 1][packet->x + 3] = (char)MazeWall::BARRICADE;

			map[packet->z + 1][packet->x + 1] = (char)MazeWall::BARRICADE;
			map[packet->z + 1][packet->x - 3] = (char)MazeWall::BARRICADE;

			map[packet->z + 3][packet->x - 1] = (char)MazeWall::BARRICADE;

			break;
		}
		}

		CStaticObjectShader::GetInstance()->AddBarricade(temp);

		//g_client[packet->id].special_skill -= 1;
		break;
	}
	case (int)MsgType::SC_ENGINEER_SPECIAL_BUILD_FAIL:
	{
		cout << "해당 지역에 지을 수 없습니다. \n";

		break;
	}
	case (int)MsgType::SC_PLAYER_SPECIAL_NUM_ZERO:
	{
		cout << "스페셜 스킬 사용 횟수가 없어 사용하지 못했습니다. \n";

		break;
	}
	case (int)MsgType::SC_PLAYER_DEAD:
	{
		sc_player_dead_packet* packet = reinterpret_cast<sc_player_dead_packet*>(ptr);

		CSoundSystem::GetInstance()->Play(L"P_Death");
		
		g_client[packet->id]._state = ClientState::DEAD;
		
		break;
	}
	case (int)MsgType::SC_PLAYER_SEARCH:
	{
		sc_search_packet* packet = reinterpret_cast<sc_search_packet*>(ptr);

		break;
	}
	case (int)MsgType::SC_UPDATE_OBJECT_INFO:
	{
		break;
	}
	case (int)MsgType::SC_UPDATE_PLAYER_INFO:
	{
		sc_player_hp_packet* packet = reinterpret_cast<sc_player_hp_packet*>(ptr);

		CSoundSystem::GetInstance()->Play(L"P_Attacked");

		g_client[packet->id].hp = packet->hp;

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

		switch (packet->map_type)
		{
		case MapType::FIRST_PATH:
		{
			r_zombie1[id].hp = packet->hp;
			r_zombie1[id]._animation = ZombieAnimationState::ATTACKED;
			CSoundSystem::GetInstance()->Play(L"zombie-hit");
			break;
		}
		case MapType::SECOND_PATH:
		{
			r_zombie2[id].hp = packet->hp;
			r_zombie2[id]._animation = ZombieAnimationState::ATTACKED;
			CSoundSystem::GetInstance()->Play(L"zombie-hit");
			break;
		}
		case MapType::FINAL_PATH:
		{
			r_zombie3[id].hp = packet->hp;
			r_zombie3[id]._animation = ZombieAnimationState::ATTACKED;
			CSoundSystem::GetInstance()->Play(L"zombie-hit");
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			b_zombie1[id].hp = packet->hp;
			b_zombie1[id]._animation = ZombieAnimationState::ATTACKED;
			CSoundSystem::GetInstance()->Play(L"zombie-hit");
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			b_zombie2[id].hp = packet->hp;
			b_zombie2[id]._animation = ZombieAnimationState::ATTACKED;
			CSoundSystem::GetInstance()->Play(L"zombie-hit");
			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			b_zombie3[id].hp = packet->hp;
			b_zombie3[id]._animation = ZombieAnimationState::ATTACKED;
			CSoundSystem::GetInstance()->Play(L"zombie-hit");
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
			r_zombie1[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::SECOND_PATH:
		{
			r_zombie2[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::FINAL_PATH:
		{
			r_zombie3[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			b_zombie1[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			b_zombie2[packet->id]._animation = ZombieAnimationState::ATTACK;
			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
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
			SetZombieInfo(r_zombie1, id, packet);
			
			//r_zombie1[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			//cout << id << "의 좌표 x : " << packet->x << ", z : " << packet->z << "\n";
			break;
		}
		case MapType::SECOND_PATH:
		{
			//move_lock.lock();
			SetZombieInfo(r_zombie2, id, packet);
			//r_zombie2[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();
			//cout << id << "의 좌표 x : " << packet->x << ", z : " << packet->z << "\n";

			break;
		}
		case MapType::FINAL_PATH:
		{
			//move_lock.lock();
			SetZombieInfo(r_zombie3, id, packet);
			//r_zombie3[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			//move_lock.lock();
			SetZombieInfo(b_zombie1, id, packet);
		//	b_zombie1[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			//cout << id << "의 좌표 x : " << packet->x << ", z : " << packet->z << "\n";

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			//move_lock.lock();
			SetZombieInfo(b_zombie2, id, packet);
			//b_zombie2[id]._animation = ZombieAnimationState::WALK;
			//move_lock.unlock();

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			//move_lock.lock();
			SetZombieInfo(b_zombie3, id, packet);
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

		if (map_type != packet->map_type)
		{
			if (packet->map_type == MapType::FIRST_PATH || packet->map_type == MapType::SECOND_PATH || packet->map_type == MapType::FINAL_PATH)
			{
				CSoundSystem::GetInstance()->StopBGM();
				CSoundSystem::GetInstance()->Play(L"in game bgm");
			}
			else if (packet->map_type == MapType::CHECK_POINT_ONE || packet->map_type == MapType::CHECK_POINT_TWO || packet->map_type == MapType::CHECK_POINT_FINAL)
			{
				CSoundSystem::GetInstance()->StopBGM();
				CSoundSystem::GetInstance()->Play(L"wavw");
			}
		}

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

			CSoundSystem::GetInstance()->Play(L"zombie-spawn");
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

			CSoundSystem::GetInstance()->Play(L"zombie-spawn");
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

			CSoundSystem::GetInstance()->Play(L"zombie-spawn");
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

			CSoundSystem::GetInstance()->Play(L"zombie-spawn");
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

			CSoundSystem::GetInstance()->Play(L"zombie-spawn");
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

			CSoundSystem::GetInstance()->Play(L"zombie-spawn");
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
	case (int)MsgType::SC_ZOMBIE_NUMBER:
	{
		sc_zombie_num_packet* packet = reinterpret_cast<sc_zombie_num_packet*>(ptr);

		remain_zombie = packet->zombie_num;

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
	case (int)MsgType::SC_ZOMBIE_ALL_KILL:
	{
		sc_zombie_all_kill_packet* packet = reinterpret_cast<sc_zombie_all_kill_packet*>(ptr);

		switch (packet->m_type)
		{
		case MapType::FIRST_PATH:
		{
			for (auto& zom : r_zombie1)
			{
				zom._state = ZombieState::DEAD;
				zom._animation = ZombieAnimationState::DEAD;
			}
			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				zom._state = ZombieState::DEAD;
				zom._animation = ZombieAnimationState::DEAD;
			}
			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				zom._state = ZombieState::DEAD;
				zom._animation = ZombieAnimationState::DEAD;
			}
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			for (auto& zom : b_zombie1)
			{
				zom._state = ZombieState::DEAD;
				zom._animation = ZombieAnimationState::DEAD;
			}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				zom._state = ZombieState::DEAD;
				zom._animation = ZombieAnimationState::DEAD;
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				zom._state = ZombieState::DEAD;
				zom._animation = ZombieAnimationState::DEAD;
			}

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
			r_zombie1[id]._animation = ZombieAnimationState::DEAD;
			r_zombie1[id]._state = ZombieState::DEAD;
			CSoundSystem::GetInstance()->Play(L"zombie-death-2");
			break;
		}
		case MapType::SECOND_PATH:
		{
			r_zombie2[id]._animation = ZombieAnimationState::DEAD;
			r_zombie2[id]._state = ZombieState::DEAD;
			CSoundSystem::GetInstance()->Play(L"zombie-death-2");
			break;
		}
		case MapType::FINAL_PATH:
		{
			r_zombie3[id]._animation = ZombieAnimationState::DEAD;
			r_zombie3[id]._state = ZombieState::DEAD;
			CSoundSystem::GetInstance()->Play(L"zombie-death-2");
			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			b_zombie1[id]._animation = ZombieAnimationState::DEAD;
			b_zombie1[id]._state = ZombieState::DEAD;
			CSoundSystem::GetInstance()->Play(L"zombie-death-2");
			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			b_zombie2[id]._animation = ZombieAnimationState::DEAD;
			b_zombie2[id]._state = ZombieState::DEAD;
			CSoundSystem::GetInstance()->Play(L"zombie-death-2");
			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			b_zombie3[id]._animation = ZombieAnimationState::DEAD;
			b_zombie3[id]._state = ZombieState::DEAD;
			CSoundSystem::GetInstance()->Play(L"zombie-death-2");
			break;
		}
		}
		break;
	}
	case (int)MsgType::SC_ZOMBIE_SEARCH:
	{
		sc_zombie_search_packet* packet = reinterpret_cast<sc_zombie_search_packet*>(ptr);

		break;
	}
	case (int)MsgType::SC_ZOMBIE_REMAIN:
	{

		break;
	}
	case (int)MsgType::SC_WAIT:
	{
		
		break;
	}
	case (int)MsgType::SC_GAME_ALL_DEAD_END:
	{

		break;
	}
	case (int)MsgType::SC_GAME_END:
	{

		break;
	}
	case (int)MsgType::SC_GM_MAP_CHANGE_MAP:
	{
		sc_gm_change_map_packet* packet = reinterpret_cast<sc_gm_change_map_packet*>(ptr);

		g_client[packet->id].x = packet->x;
		g_client[packet->id].z = packet->z;

		break;
	}
	case(int)MsgType::SC_GM_PLAYER_HP_UP:
	{
		sc_gm_player_hp_packet* packet = reinterpret_cast<sc_gm_player_hp_packet*>(ptr);

		g_client[packet->id].hp = packet->hp;

		break;
	}
	default:
	{
		//cout << packet_type << "으로 잘못왔음 \n";
		break;
	}
	}
	
}

void Network::send_login_packet()
{
	cs_login_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_LOGIN_REQUEST;
	_socket.do_send(sizeof(packet), &packet);
}

void volatile Network::ProcessData(Exp_Over& exp_over, int& size)
{
	int remain_data = size + _prev_size;
	unsigned char* packet_start = exp_over._net_buf;
	atomic<int> packet_size = (int)((packet_start[1] * 256) + packet_start[0]);

	while (packet_size <= remain_data)
	{
		ProcessPacket(packet_start);
		remain_data -= packet_size;
		packet_start += packet_size;
		if (remain_data > 0)
		{
			int temp = (int)((packet_start[1] * 256) + packet_start[0]);

			packet_size = temp;
			if (packet_size > 850)
			{
				packet_size = 0;
				remain_data = 0;
				_prev_size = 0;
				break;
			}
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

void Network::send_player_move_packet(float t_x, float t_z, float speed, float x, float z, float rotation, bool input)
{
	cs_move_packet packet;
	packet.t_x = t_x;
	packet.t_z = t_z;
	packet.speed = speed;
	packet.x = x;
	packet.z = z;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_PLAYER_MOVE;
	packet.in_input = input;
	packet.rotation_angle = rotation;
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
	send_player_move_packet(g_client[p_id].t_x, g_client[p_id].t_z, g_client[p_id].speed, g_client[p_id].x, g_client[p_id].z, g_client[p_id].rotation_angle, g_client[p_id].is_input);

	AddTimer(p_id, EVENT_TYPE::PLAYER_MOVE, 100);
}

void Network::Login()
{
	while (true) {
		send_login_packet();

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

}

void Network::Player_Select(PlayerType type)
{
	send_player_select_packet(type);
	this_thread::sleep_for(10ms);

	Send_request_packet(MsgType::CS_BARRICADE_REQUEST);
}

void Network::Work()
{
	// 0. 클라이언트 초기화
	Initialize();

	// 1. 로그인 요청
	Login();

	int select;
	PlayerType type = PlayerType::NONE;
	select_complete = false;
	while (1)
	{
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
		}

		// 2. 플레이어 캐릭터 선택
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

			Player_Select(type);
			this_thread::sleep_for(10ms);
			continue;
		}
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

		//if (attack_time < chrono::system_clock::now() && attack_state == true)
		//{
		//	attack_state = false;
		//	attack_time = chrono::system_clock::now() + 250ms;
		//}

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
	zombie.x = x;
	zombie.z = z;
	/*int row = z;
	int col = x;

	float row_result = z - row;
	float col_result = x - col;

	if (row_result >= 0.5f) row += 1;
	if (col_result >= 0.5f) col += 1;

	if (!zombie.IsCollied(row, col, map))
	{
		zombie.x = x;
		zombie.z = z;
	}*/
}
//float test_time = 0.0f;

void Network::Update(float time_elapsed)
{
	for (auto & player : g_client) {
		
		if (player._state != ClientState::INGAME) continue;

		player.move(time_elapsed);
	}
	
	switch (map_type)
	{
	case MapType::FIRST_PATH:
	{
		UpdateZombies(r_zombie1, time_elapsed);
		break;
	}
	case MapType::SECOND_PATH:
	{
		UpdateZombies(r_zombie2, time_elapsed);
		break;
	}
	case MapType::FINAL_PATH:
	{
		UpdateZombies(r_zombie3, time_elapsed);
		break;
	}
	case MapType::CHECK_POINT_ONE:
	{
		UpdateZombies(b_zombie1, time_elapsed);
		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		UpdateZombies(b_zombie2, time_elapsed);
		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		UpdateZombies(b_zombie3, time_elapsed);
		break;
	}
	}
}

template<typename Arr>
void Network::UpdateZombies(Arr& arr, float time_elapsed)
{
	for (auto& zombie : arr)
	{
		if (zombie._state != ZombieState::SPAWN) continue;

		ZombieAngle(zombie, time_elapsed);

		if (zombie.arrive) continue;

		ZombieMove(zombie, time_elapsed);
	}
}

template<typename Arr>
static void Network::SetZombieInfo(Arr& arr, unsigned int id, sc_zombie_move_packet* packet)
{
	if (abs(arr[id].x - packet->x) + abs(arr[id].z - packet->z) > 20) {
		arr[id].x = packet->x;
		arr[id].z = packet->z;
		cout << "좌표 바뀜" << endl;
	}
	arr[id].speed = packet->speed;
	arr[id].t_x = packet->t_x;
	arr[id].t_z = packet->t_z;
	arr[id].dir = packet->dir;
	arr[id].arrive = false;
}