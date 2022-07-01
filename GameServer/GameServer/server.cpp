#include "server.h"

Socket Server::_socket;
Exp_Over* Server::_exp_over;

CGameTimer Server::game_timer;

array<Client, MAX_PLAYER> Server::g_clients;

array<NPC , ROAD_ZOMBIE_NUM> Server::r_zombie1;
array<NPC , ROAD_ZOMBIE_NUM> Server::r_zombie2;
array<NPC , ROAD_ZOMBIE_NUM> Server::r_zombie3;
		
array<NPC , FIRST_CHECK_POINT_ZOMBIE_NUM> Server::b_zombie1;
array<NPC , TWO_CHECK_POINT_ZOMBIE_NUM> Server::b_zombie2;
array<NPC , THREE_CHECK_POINT_ZOMBIE_NUM> Server::b_zombie3;

Map Server::map;
MapType Server::map_type;
AS_Map Server::as_map;

priority_queue <timer_event> Server::timer_queue;
mutex Server::timer_lock;
mutex Server::num_lock;
mutex Server::spawn_lock;
mutex Server::map_lock;

//chrono::system_clock::time_point Server::start_time;
//chrono::system_clock::time_point Server::end_time;
//chrono::system_clock::time_point Server::zombie_start_time;
//chrono::system_clock::time_point Server::zombie_end_time;

chrono::milliseconds Server::sec;
float Server::s_speed;
float Server::z_speed;

bool Server::zombie_send;
int Server::door_num;
bool Server::game_start;
Server::Server()
{
	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	int retval = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (retval != 0)
		throw Exception("Start Fail");
	
	_socket.Init();
	_socket.Bind();
	_socket.Listen();
	_socket.CreatePort();

	for (int i = 0; i < MAX_PLAYER; ++i) {
		g_clients[i]._id = i;
	}
	 //서버 초기화 설정 후... 미리 집어올 파일들 여기서 불러오기?
}

Server::~Server()
{
}

void Server::Initialize()
{
	game_timer.Start();

	zombie_send = false;
	game_start = false;

	// 맵 파일을 읽어서 충돌체크용 맵 파일 제작
	map.ReadMapFile();
	map.Initialize();

	// 좀비의 이동 알고리즘인 A* 알고리즘을 사용하기 위해 맵을 맞게 초기화
	// 바리게이트가 필요하기 때문에 앞서 바리게이트를 제작하고 초기화를 진행함
	as_map.InitMap(map);

	// 좀비 스폰 위치 초기화
	PlaceZombie(MapType::FIRST_PATH);
	PlaceZombie(MapType::SECOND_PATH);
	PlaceZombie(MapType::FINAL_PATH);
	PlaceZombie(MapType::CHECK_POINT_ONE);
	PlaceZombie(MapType::CHECK_POINT_TWO);
	PlaceZombie(MapType::CHECK_POINT_FINAL);

	door_num = 0;
}

float Server::Distance(float s_x, float s_z, float e_x, float e_z)
{
	float distance = (float)sqrt(((e_x - s_x)*(e_x - s_x)) + ((e_z - s_z)*(e_z - s_z)));

	return distance;
}

void Server::Disconnect(int c_id)
{
	Client& cl = g_clients[c_id];

	cl.send_start_packet = false;

	//cl.map_lock.lock();
	cl.map_type = MapType::SPAWN;
	//cl.map_lock.unlock();

	//cl.type_lock.lock();
	cl._type = PlayerType::NONE;
	//cl.type_lock.unlock();

	//cl.state_lock.lock();
	closesocket(cl.m_socket.s_socket);
	cl._state = ClientState::FREE;
	//cl.state_lock.unlock();
}

int Server::NewID()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
	//	g_clients[i].state_lock.lock();
		if (ClientState::FREE == g_clients[i]._state) {
			g_clients[i]._state = ClientState::ACCEPT;
			//g_clients[i].state_lock.unlock();
			return i;
		}
		//g_clients[i].state_lock.unlock();
	}
	cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

void Server::Send_login_ok_packet(int c_id)
{
	sc_login_ok_packet packet;
	packet.size = sizeof(packet);
	packet.id = c_id;
	packet.type = MsgType::SC_LOGIN_OK;
	strcpy_s(packet.name, g_clients[c_id].player_name);
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_select_packet(int c_id, int s_id)
{
	sc_player_select_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_SELECT;
	packet.playertype = g_clients[c_id]._type;
	packet.hp = g_clients[c_id].player->hp;
	packet.maxhp = g_clients[c_id].player->maxhp;
	packet.shp = g_clients[c_id].player->shp;
	packet.maxshp = g_clients[c_id].player->maxshp;
	packet.x = g_clients[c_id].player->x;
	packet.z = g_clients[c_id].player->z;
	packet.bullet = g_clients[c_id].player->bullet;
	packet.speed = g_clients[c_id].player->speed;
	g_clients[s_id].do_send(sizeof(packet), &packet);
}

void Server::Send_player_move_packet(int c_id, int s_id, float x, float z, float t_x, float t_z, float speed)
{
	sc_player_move_packet packet;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_MOVE;
	packet.x = x;
	packet.z = z;
	packet.t_x = t_x;
	packet.t_z = t_z;
	packet.speed = speed;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_search_packet(int c_id, int x, int z, ObjectType _type)
{
	sc_search_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_SEARCH;
	packet.obj_type = _type;
	packet.x = x;
	packet.z = z;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_fail_packet(int c_id, MsgType _type)
{
	sc_fail_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = _type;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_door_open_packet(int c_id, int s_id, int row, int col, int size_x, int size_z)
{
	sc_door_open_packet packet;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_DOOR_OPEN;
	packet.row = row;
	packet.col = col;
	packet.size_x = size_x;
	packet.size_z = size_z;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_barricade_packet(int c_id)
{
	sc_barricade_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_BARRICADE_SEND;
	memcpy(packet.one_base, map.one_base, sizeof(map.one_base));
	memcpy(packet.two_base, map.two_base, sizeof(map.two_base));
	memcpy(packet.three_base, map.three_base, sizeof(map.three_base));
	memcpy(packet.three_base2, map.three_base2, sizeof(map.three_base2));
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_game_start_packet(int c_id)
{
	sc_game_start_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_GAME_START;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

bool Server::MapCheck(MapType map_type)
{
	for (auto& cl : g_clients)
	{
		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			continue;
		}
		//cl.state_lock.unlock();

		//cl.map_lock.lock();
		if (cl.map_type != map_type)
		{
			//cl.map_lock.unlock();
			return false;
		}
		//cl.map_lock.unlock();
	}

	return true;
}

void Server::InitZombie(NPC& npc, int &i, float &PosX, float &PosZ)
{
	int random_zombie_type = rand() % 4;
	switch (random_zombie_type)
	{
	case 0:
		npc.zombie = new NormalZombie;
		npc._id = i;
		npc.zombie->SetX(PosX);
		npc.zombie->SetZ(PosZ);
		break;
	case 1:
		npc.zombie = new SoldierZombie;
		npc._id = i;
		npc.zombie->SetX(PosX);
		npc.zombie->SetZ(PosZ);
		break;
	case 2:
		npc.zombie = new TankerZombie;
		npc._id = i;
		npc.zombie->SetX(PosX);
		npc.zombie->SetZ(PosZ);
		break;
	case 3:
		npc.zombie = new DogZombie;
		npc._id = i;
		npc.zombie->SetX(PosX);
		npc.zombie->SetZ(PosZ);
		break;
	}

	//cout << npc.zombie->GetX() - 30 << ", " << npc.zombie->GetZ() - 30 << "\n";
}

void Server::PlaceZombie(MapType m_type)
{
	random_device rd;
	default_random_engine dre{ rd() };
	{
		switch (m_type)
		{
		case MapType::FIRST_PATH:
			for (int i = 0; i < ROAD_ZOMBIE_NUM; ++i)
			{
				uniform_int_distribution<> random_road1{ One_Road_Pos.x + 1, One_Road_Pos3.x + 22 };
				float x = (float)random_road1(dre);
				float z = 0.0f;
				if (One_Road_Pos.x + 1 <= x && x < One_Road_Pos.x + 14)
				{
					uniform_int_distribution<>random_road2{ One_Road_Pos.z+2, One_Road_Pos2.z + 21 };
					z = (float)random_road2(dre);
				}
				else if (One_Road_Pos.x + 14 <= x && x < One_Road_Pos3.x)
				{
					uniform_int_distribution<>random_road2{ One_Road_Pos2.z + 1, One_Road_Pos2.z + 21 };
					z = (float)random_road2(dre);
				}
				else if (One_Road_Pos3.x <= x && x < One_Road_Pos3.x + 23)
				{
					uniform_int_distribution<>random_road2{ One_Road_Pos2.z + 1, One_Base_Pos.z-2 };
					z = (float)random_road2(dre);
				}

				if (map.map[(int)z][(int)x] != (char)MazeWall::ROAD)
				{
					i--;
					continue;
				}

				InitZombie(r_zombie1[i], i, x, z);
				r_zombie1[i]._state = ZombieState::SLEEP;
			}

			/*
			while (1)
			{
				bool sort_complete = true;
				for (int num = 0; num < ROAD_ZOMBIE_NUM - 1; ++num)
				{
					float DisA = r_zombie1[r_zombie1_id[num]]._distance;
					float DisB = r_zombie1[r_zombie1_id[num + 1]]._distance;

					if (DisA < DisB)
					{
						continue;
					}
					else
					{
						int temp = r_zombie1_id[num];
						r_zombie1_id[num] = r_zombie1_id[num + 1];
						r_zombie1_id[num + 1] = temp;
						sort_complete = false;
					}
				}
				if (sort_complete)
					break;
			}
			*/

			break;
		case MapType::SECOND_PATH:
			for (int i = 0; i < ROAD_ZOMBIE_NUM; ++i)
			{
				uniform_int_distribution<> random_road_x{ Two_Road_Pos.x + 1, TWO_Base_Pos.x-3 };
				uniform_int_distribution<> random_road_z{ Two_Road_Pos.z, Two_Road_Pos.z + 24 };

				float x = (float)random_road_x(dre);
				float z = (float)random_road_z(dre);

				if (map.map[(int)z][(int)x] != (char)MazeWall::ROAD)
				{
					i--;
					continue;
				}

				InitZombie(r_zombie2[i], i, x, z);
				r_zombie2[i]._state = ZombieState::SLEEP;
			}

			break;
		case MapType::FINAL_PATH:
			for (int i = 0; i < ROAD_ZOMBIE_NUM; ++i)
			{
				uniform_int_distribution<> random_road_x{ Three_Road_Pos.x+1, THREE_Base_Pos.x - 3 };
				uniform_int_distribution<> random_road_z{ Three_Road_Pos.z, Three_Road_Pos.z + 24 };

				float x = (float)random_road_x(dre);
				float z = (float)random_road_z(dre);

				if (map.map[(int)z][(int)x] != (char)MazeWall::ROAD)
				{
					i--;
					continue;
				}

				InitZombie(r_zombie3[i], i, x, z);
				r_zombie3[i]._state = ZombieState::SLEEP;
			}

			break;
		case MapType::CHECK_POINT_ONE:
			for (int i = 0; i < FIRST_CHECK_POINT_ZOMBIE_NUM; ++i)
			{
				uniform_int_distribution<> random_road_x{ One_Base_Pos.x + 1, One_Base_End_Pos.x - 1 };
				float x = (float)random_road_x(dre);
				float z = 0;

				if ((One_Base_Pos.x + 1 <= x && x < One_Base_Pos.x + 5) || (One_Base_End_Pos.x-6 < x && x <= One_Base_End_Pos.x-1))
				{
					uniform_int_distribution<> random_road_z{ One_Base_Pos.z + 1, One_Base_End_Pos.z-1 };
					z = (float)random_road_z(dre);
				}
				else
				{
					uniform_int_distribution<> top_down{ 0,1 };
					int random = top_down(dre);
					if (random == 0)
					{
						uniform_int_distribution<> random_road_z{ One_Base_Pos.z + 1, One_Base_Pos.z + 5 };
						z = (float)random_road_z(dre);
					}
					else
					{
						uniform_int_distribution<> random_road_z{ One_Base_End_Pos.z - 6, One_Base_End_Pos.z-1 };
						z = (float)random_road_z(dre);
					}
				}

				if (map.map[(int)z][(int)x] != (char)MazeWall::ROAD)
				{
					i--;
					continue;
				}

				InitZombie(b_zombie1[i], i, x, z);
				b_zombie1[i]._state = ZombieState::SLEEP;
			}

			break;
		case MapType::CHECK_POINT_TWO:
			for (int i = 0; i < TWO_CHECK_POINT_ZOMBIE_NUM; ++i)
			{
				uniform_int_distribution<> random_road_x{ TWO_Base_Pos.x + 1, TWO_Base_End_Pos.x-1 };
				float x = (float)random_road_x(dre);
				float z = 0;

				if ((TWO_Base_Pos.x+1 <= x && x < TWO_Base_Pos.x + 5) || (TWO_Base_End_Pos.x - 6 < x && x <= TWO_Base_End_Pos.x-1))
				{
					uniform_int_distribution<> random_road_z{ TWO_Base_Pos.z+1, TWO_Base_End_Pos.z -1 };
					z = (float)random_road_z(dre);
				}
				else
				{
					uniform_int_distribution<> top_down{ 0,1 };
					int random = top_down(dre);
					if (random == 0)
					{
						uniform_int_distribution<> random_road_z{ TWO_Base_Pos.z+1, TWO_Base_Pos.z + 5 };
						z = (float)random_road_z(dre);
					}
					else
					{
						uniform_int_distribution<> random_road_z{ TWO_Base_End_Pos.z -6, TWO_Base_End_Pos.z - 1 };
						z = (float)random_road_z(dre);
					}
				}

				if (map.map[(int)z][(int)x] != (char)MazeWall::ROAD)
				{
					i--;
					continue;
				}

				InitZombie(b_zombie2[i], i, x, z);
				b_zombie2[i]._state = ZombieState::SLEEP;
			}

			break;
		case MapType::CHECK_POINT_FINAL:
			for (int i = 0; i < THREE_CHECK_POINT_ZOMBIE_NUM; ++i)
			{
				uniform_int_distribution<> random_road_x{ THREE_Base_Pos.x+1, THREE_Base_End_Pos2.x-1 };
				float x = (float)random_road_x(dre);
				float z = 0;

				if ((THREE_Base_Pos.x+1 <= x && x < THREE_Base_Pos.x + 6) || (THREE_Base_End_Pos2.x - 6 <= x && x < THREE_Base_End_Pos2.x-1))
				{
					uniform_int_distribution<> random_road_z{ THREE_Base_Pos.z+1, THREE_Base_End_Pos.z-1 };
					z = (float)random_road_z(dre);
				}
				else
				{
					uniform_int_distribution<> top_down{ 0,1 };
					int random = top_down(dre);
					if (random == 0)
					{
						uniform_int_distribution<> random_road_z{ THREE_Base_Pos.z+1, THREE_Base_Pos.z + 5 };
						z = (float)random_road_z(dre);
					}
					else
					{
						uniform_int_distribution<> random_road_z{ THREE_Base_End_Pos.z - 6, THREE_Base_End_Pos.z - 1 };
						z = (float)random_road_z(dre);
					}
				}

				if (map.map[(int)z][(int)x] != (char)MazeWall::ROAD)
				{
					i--;
					continue;
				}

				InitZombie(b_zombie3[i], i, x, z);
				b_zombie3[i]._state = ZombieState::SLEEP;
			}

			break;
		}
	}
}

void Server::ChangeMapType(Client& cl)
{
	MapType change = MapType::NONE;
	switch (cl.map_type)
	{
	case MapType::SPAWN:
		if ((One_Road_Pos.x < cl.player->x && cl.player->x < One_Road_Pos3.x) && (One_Road_Pos.z < cl.player->z && cl.player->z <One_Base_Pos.z - 1))
		{
			change = MapType::FIRST_PATH;
			//cl.map_lock.lock();
			cl.map_type = MapType::FIRST_PATH;
			//cl.map_lock.unlock();

			if (zombie_send == false)
			{
				for (auto& s_cl : g_clients)
				{
					//s_cl.state_lock.lock();
					if (s_cl._state != ClientState::INGAME)
					{
						//s_cl.state_lock.unlock();
						continue;
					}
					//s_cl.state_lock.unlock();

					AddTimer(s_cl._id, EVENT_TYPE::EVENT_NPC_SEND, 33);
				}

				zombie_send = true;
			}
		}

		cl.zombie_list.clear();

		break;
	case MapType::FIRST_PATH:
		if ((One_Base_Pos.x < cl.player->x && cl.player->x < One_Base_End_Pos.x) && (One_Base_Pos.z < cl.player->z && cl.player->z < One_Base_End_Pos.z))
		{
			change = MapType::CHECK_POINT_ONE;
			//cl.map_lock.lock();
			cl.map_type = MapType::CHECK_POINT_ONE;
			//cl.map_lock.unlock();
		}

		cl.zombie_list.clear();
		break;
	case MapType::SECOND_PATH:
		if ((TWO_Base_Pos.x < cl.player->x && cl.player->x < TWO_Base_End_Pos.x) && (TWO_Base_Pos.z < cl.player->z && cl.player->z < TWO_Base_End_Pos.z))
		{
			change = MapType::CHECK_POINT_TWO;
			//cl.map_lock.lock();
			cl.map_type = MapType::CHECK_POINT_TWO;
			//cl.map_lock.unlock();
		}

		cl.zombie_list.clear();
		break;
	case MapType::FINAL_PATH:
		if ((THREE_Base_Pos.x < cl.player->x && cl.player->x < THREE_Base_End_Pos2.x) && (THREE_Base_Pos.z < cl.player->z && cl.player->z < THREE_Base_End_Pos.z))
		{
			change = MapType::CHECK_POINT_FINAL;
			//cl.map_lock.lock();
			cl.map_type = MapType::CHECK_POINT_FINAL;
			//cl.map_lock.unlock();
		}

		cl.zombie_list.clear();
		break;
	case MapType::CHECK_POINT_ONE:
		if ((Two_Road_Pos.x < cl.player->x && cl.player->x < TWO_Base_Pos.x ) && (Two_Road_Pos.z < cl.player->z && cl.player->z < Two_Road_Pos.z + ROAD_SIZE))
		{
			change = MapType::SECOND_PATH;
			//cl.map_lock.lock();
			cl.map_type = MapType::SECOND_PATH;
			//cl.map_lock.unlock();
		}

		cl.zombie_list.clear();
		break;
	case MapType::CHECK_POINT_TWO:
		if ((Three_Road_Pos.x < cl.player->x && cl.player->x <THREE_Base_Pos.x ) && (Three_Road_Pos.z < cl.player->z && cl.player->z < Three_Road_Pos.z + ROAD_SIZE))
		{
			change = MapType::FINAL_PATH;
			//cl.map_lock.lock();
			cl.map_type = MapType::FINAL_PATH;
			//cl.map_lock.unlock();
		}

		cl.zombie_list.clear();
		break;
	case MapType::CHECK_POINT_FINAL:
		if ((Exit_Pos.x < cl.player->x && cl.player->x < Exit_End_Pos.x) && (Exit_Pos.z < cl.player->z && cl.player->z < Exit_End_Pos.z))
		{
			change = MapType::EXIT;
			//cl.map_lock.lock();
			cl.map_type = MapType::EXIT;
			//cl.map_lock.unlock();
		}
		break;
	}

	if (MapCheck(cl.map_type))
	{
		//cout << "좀비를 스폰시킵니다 \n";
		switch (map_type)
		{
			case MapType::SECOND_PATH:
			{
				for (auto& zom : b_zombie1)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieDead(zom, MapType::CHECK_POINT_ONE);
					}
				}

				break;
			}
			case MapType::FINAL_PATH:
			{
				for (auto& zom : b_zombie2)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieDead(zom, MapType::CHECK_POINT_TWO);
					}
				}

				break;
			}
			case MapType::CHECK_POINT_ONE:
			{
				for (auto& zom : r_zombie1)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieDead(zom, MapType::FIRST_PATH);
					}
				}

				break;
			}
			case MapType::CHECK_POINT_TWO:
			{
				for (auto& zom : r_zombie2)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieDead(zom, MapType::SECOND_PATH);
					}
				}

				break;
			}
			case MapType::CHECK_POINT_FINAL:
			{
				for (auto& zom : r_zombie3)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieDead(zom, MapType::FINAL_PATH);
					}
				}

				break;
			}
		}

		map_type = cl.map_type;

		if (change == MapType::CHECK_POINT_ONE || change == MapType::CHECK_POINT_TWO || change == MapType::CHECK_POINT_FINAL)
			AddTimer(0, EVENT_TYPE::EVENT_NPC_SPAWN, 4000);
	}
}

void Server::Send_player_attack_packet(int c_id, int a_id)
{
	sc_player_attack_packet packet;
	packet.size = sizeof(packet);
	packet.id = a_id;
	packet.type = MsgType::SC_PLAYER_ATTACK;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_player_bullet_info_packet(int c_id, int bullet)
{
	sc_player_bullet_info_packet packet;
	packet.bullet = bullet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_BULLET_INFO;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_zombie_info_packet(int c_id, int z_id, int hp, MapType m_type)
{
	sc_update_zombie_info_packet packet;
	packet.hp = hp;
	packet.id = z_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_UPDATE_ZOMBIE_INFO;
	packet.map_type = m_type;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

void Server::Send_player_reload_packet(int c_id, int s_id, int bullet)
{
	sc_player_reload_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_RELOAD;
	packet.bullet = bullet;
	packet.id = s_id;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_viewlist_put_packet(int c_id, int z_id, MapType m_type, float z_x, float z_z, MsgType msg, ZombieType z_type)
{
	sc_zombie_viewlist_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_VIEWLIST_PUT;
	packet.z_id = z_id;
	packet.m_type = m_type;
	packet.x = z_x;
	packet.z = z_z;
	packet.animation = msg;
	packet.z_type = z_type;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_viewlist_remove_packet(int c_id, int z_id, MapType m_type)
{
	sc_zombie_viewlist_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_VIEWLIST_REMOVE;
	packet.z_id = z_id;
	packet.m_type = m_type;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

bool Server::ZombieRemain(NPC& npc)
{
	if (npc._state != ZombieState::DEAD)				// 좀비가 죽어있지 않다면 아직 해당 길이나 거점을 해결하지 않은 것
	{
		for (auto& client : g_clients)
		{
			//client.state_lock.lock();
			if (client._state != ClientState::INGAME)
			{
				//client.state_lock.unlock();
				continue;
			}
			//client.state_lock.unlock();

			Send_fail_packet(client._id, MsgType::SC_ZOMBIE_REMAIN);	// 그래서 좀비가 남아있다고 전송
		}

		return true;	// 한마리라도 남아있으면 일단 못깬거니 바로 true반환
	}

	return false;
}

void Server::Send_player_rotate_packet(int c_id, int s_id, float m_x, float m_z)
{
	sc_player_rotate_packet packet;
	packet.size = sizeof(packet);
	packet.id = s_id;
	packet.type = MsgType::SC_PLAYER_ROTATE;
	packet.mx = m_x;
	packet.mz = m_z;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::PlayerAttack(Client& cl, NPC& npc, MapType m_type, float p_x, float p_z)
{
	// 총알이 소모되고 공격을 하였기 때문에 모두에게 공격이 되었다고 전달
	for (auto& a_cl : g_clients)
	{
		if (a_cl._state != ClientState::INGAME) continue;

		Send_player_attack_packet(a_cl._id, cl._id);
	}
	
	bool collied_zombie = cl.player->PlayerAttack(p_x, p_z, npc.zombie->GetX(), npc.zombie->GetZ());

	if (collied_zombie)
	{
		if (cl.player->z < npc.zombie->GetZ())
		{
			if (p_z <= 0)
			{
				collied_zombie = true;
			}
			else
			{
				collied_zombie = false;
			}
		}
		else
		{
			if (p_z <= 0)
			{
				collied_zombie = false;
			}
			else
			{
				collied_zombie = true;
			}
		}
	}

	int hp = npc.zombie->hp;

	if (collied_zombie)
	{
		//zom.zombie->z_attack_lock.lock();
		hp -= cl.player->attack;

		if (hp <= 0)
		{
			hp = 0;
			AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_DEAD, 0);
			
			//ZombieDead(npc, m_type);
		}

		//zom.zombie->z_attack_lock.unlock();

		Send_zombie_info_packet(cl._id, npc._id, hp, m_type);
	}

	if(npc._state == ZombieState::SPAWN)
		npc.zombie->hp = hp;
}

void Server::CommanderSpecialSkill(Client& cl)
{

}

void Server::EngineerSpecialSkill(Client& cl)
{

}

void Server::MercenarySpecialSkill(Client& cl)
{

}

void Server::ProcessPacket(int client_id, unsigned char* p)
{
	unsigned char packet_type = p[2];
	Client& cl = g_clients[client_id];
	bool check = false;

	switch (packet_type)
	{
	case (int)MsgType::CS_LOGIN_REQUEST:
	{
		cs_login_packet* packet = reinterpret_cast<cs_login_packet*>(p);

		for (auto& other : g_clients)
		{
			//other.state_lock.lock();
			if (other._state == ClientState::FREE)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			if (strcmp(other.player_name, packet->name) == 0)
			{
				Send_fail_packet(client_id, MsgType::SC_LOGIN_FAIL);
				check = true;
				break;
			}
		}

		if (check)
			break;

		strcpy_s(cl.player_name, packet->name);
		Send_login_ok_packet(client_id);

		// 클라이언트가 접속하였으므로 INGAME 상태로 변환
		Client& cl = g_clients[client_id];
		//cl.state_lock.lock();
		cl._state = ClientState::INGAME;
		//cl.state_lock.unlock();

		// 클라이언트가 접속한 시점에는 스폰지점에 있으므로 스폰지점으로 설정
		//cl.map_lock.lock();
		cl.map_type = MapType::SPAWN;
		//cl.map_lock.unlock();

		// 다른 플레이어에게 로그인 아이디, 플레이어 네임 보내기
		
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			sc_login_other_packet login_packet;
			login_packet.id = client_id;
			strcpy_s(login_packet.name, cl.player_name);
			login_packet.size = sizeof(login_packet);
			login_packet.type = MsgType::SC_LOGIN_OTHER;
			other.do_send(sizeof(login_packet), &login_packet);
		}

		// 자기 자신에게 다른 플레이어들의 아이디, 네임 받아오기
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			sc_login_other_packet login_packet;
			login_packet.id = other._id;
			login_packet.size = sizeof(login_packet);
			strcpy_s(login_packet.name, other.player_name);
			login_packet.type = MsgType::SC_LOGIN_OTHER;
			cl.do_send(sizeof(login_packet), &login_packet);
		}

		break;
	}
	case (int)MsgType::CS_PLAYER_ATTACK:
	{
		cs_attack_packet* packet = reinterpret_cast<cs_attack_packet*>(p);

		if (cl._state != ClientState::INGAME) break;
		if (cl.player->bullet == 0)
		{
			// 플레이어의 총알이 모두 떨어졌으므로 재장전해야합니다.
			Send_fail_packet(cl._id, MsgType::SC_PLAYER_RELOAD_REQUEST);
			return;
		}

		cl.player->bullet -= 1;

		Send_player_bullet_info_packet(cl._id, cl.player->bullet);

		switch (cl.map_type)
		{
		case MapType::FIRST_PATH:
		{
			for (auto& zom : r_zombie1)
			{
				if (zom._state != ZombieState::SPAWN) continue;

				PlayerAttack(cl, zom, MapType::FIRST_PATH, packet->x, packet->z);
			}

			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				if (zom._state != ZombieState::SPAWN) continue;

				PlayerAttack(cl, zom, MapType::SECOND_PATH, packet->x, packet->z);
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				if (zom._state != ZombieState::SPAWN) continue;

				PlayerAttack(cl, zom, MapType::FINAL_PATH, packet->x, packet->z);
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			for (auto& zom : b_zombie1)
			{
				if (zom._state != ZombieState::SPAWN) continue;

				PlayerAttack(cl, zom, MapType::CHECK_POINT_ONE, packet->x, packet->z);
			}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				if (zom._state != ZombieState::SPAWN) continue;

				PlayerAttack(cl, zom, MapType::CHECK_POINT_TWO, packet->x, packet->z);
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				if (zom._state != ZombieState::SPAWN) continue;

				PlayerAttack(cl, zom, MapType::CHECK_POINT_FINAL, packet->x, packet->z);
			}

			break;
		}
		}

		cl.idle_time = chrono::system_clock::now();
		break;
	}
	case (int)MsgType::CS_PLAYER_BUILD:
	{
		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			break;
		}
		//cl.state_lock.unlock();

		cl.idle_time = chrono::system_clock::now();
		break;
	}
	case (int)MsgType::CS_PLAYER_CHAT:
	{
		cs_chat_packet* packet = reinterpret_cast<cs_chat_packet*>(p);

		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			break;
		}
		//cl.state_lock.unlock();

		cout << "mess : " << packet->message << "\n";

		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			sc_player_chat_packet chat_packet;
			chat_packet.id = other._id;
			memcpy(chat_packet.message, packet->message, MAX_CHAT_SIZE);
			chat_packet.size = sizeof(chat_packet);
			chat_packet.s_id = client_id;
			chat_packet.type = MsgType::SC_PLAYER_CHAT;

			other.do_send(sizeof(chat_packet), &chat_packet);
		}

		cl.idle_time = chrono::system_clock::now();
		break;
	}
	case (int)MsgType::CS_PLAYER_HIDE:
	{
		break;
	}
	case (int)MsgType::CS_PLAYER_INTERATION:
	{
		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			break;
		}
		//cl.state_lock.unlock();
		// 여기서 주변 사물을 검색해야함
		// 아직은 DOOR 밖에 존재하지 않아 해당 코드만 작성

		// 먼저 DOOR는 엔지니어만 작동 가능하여 엔지니어가 아니라면 입력해도 아무런 효과가 없고 나가야함
		//cl.type_lock.lock();
		if (cl._type != PlayerType::ENGINEER)
		{
			Send_fail_packet(cl._id, MsgType::SC_PLAYER_NOT_ENGINEER);
			//cl.type_lock.unlock();
			break;
		}
		//cl.type_lock.unlock();

		switch (map_type)
		{
		case MapType::FIRST_PATH:
		{
			//for (auto& zom : r_zombie1)
			//{
			//	if (ZombieRemain(zom))		// 그래서 만약에 남아있으면 true이므로 함수를 그냥 빠져나온다.
			//		return;
			//}

			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				if (ZombieRemain(zom))		// 그래서 만약에 남아있으면 true이므로 함수를 그냥 빠져나온다.
					return;
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				if (ZombieRemain(zom))		// 그래서 만약에 남아있으면 true이므로 함수를 그냥 빠져나온다.
					return;
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			//for (auto& zom : b_zombie1)
			//{
			//	if (ZombieRemain(zom))		// 그래서 만약에 남아있으면 true이므로 함수를 그냥 빠져나온다.
			//		return;
			//}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				if (ZombieRemain(zom))		// 그래서 만약에 남아있으면 true이므로 함수를 그냥 빠져나온다.
					return;
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				if (ZombieRemain(zom))		// 그래서 만약에 남아있으면 true이므로 함수를 그냥 빠져나온다.
					return;
			}

			break;
		}
		}
		
		AddTimer(cl._id, EVENT_TYPE::EVENT_DOOR_OPEN, 4000);
		cl.idle_time = chrono::system_clock::now();

		break;
	}
	case (int)MsgType::CS_PLAYER_RELOAD_REQUEST:
	{
		//cl.bullet_lock.lock();
		cl.player->bullet = 30;
		//cl.bullet_lock.unlock();

		for (auto& s_cl : g_clients)
		{
			//s_cl.state_lock.lock();
			if (s_cl._state != ClientState::INGAME)
			{
			//	s_cl.state_lock.unlock();
				continue;
			}
			//s_cl.state_lock.unlock();

			Send_player_reload_packet(s_cl._id, cl._id, cl.player->bullet);
		}
		break;
	}
	case (int)MsgType::CS_PLAYER_ROTATE:
	{
		if (cl._state != ClientState::INGAME)
			break;

		cs_rotate_packet* packet = reinterpret_cast<cs_rotate_packet*>(p);

		for (auto& s_cl : g_clients)
		{
			if (s_cl._state != ClientState::INGAME)
				continue;

			if (s_cl._id == cl._id)
				continue;

			Send_player_rotate_packet(s_cl._id, cl._id, packet->mx, packet->mz);
		}
		break;
	}
	case (int)MsgType::CS_PLAYER_MOVE:
	{
		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			break;
		}
		//cl.state_lock.unlock();

		cs_move_packet* packet = reinterpret_cast<cs_move_packet*>(p);

		for (auto &other : g_clients)
		{
			if (other._id == client_id) continue;
			if (other._state != ClientState::INGAME) continue;

			Send_player_move_packet(other._id, client_id, packet->x, packet->z, packet->t_x, packet->t_z, packet->speed);
		}

		cl.move_lock.lock();
		int pre_x = (int)cl.player->x;
		int pre_z = (int)cl.player->z;
		cl.player->x = packet->x;
		cl.player->z = packet->z;
		cl.move_lock.unlock();


		// 미리 주변 뷰리스트 확인
		unordered_set<int> near_zombie_list;
		switch (cl.map_type)
		{
		case MapType::FIRST_PATH:
		{
			for (auto& zom : r_zombie1)
			{
				if (zom._state != ZombieState::SPAWN)	// 좀비가 깨어있지 않으므로 뷰리스트에 넣을일이 없다.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// 좀비가 깨어있어도 뷰리스트 범위 밖이면 그릴일이 없다.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				if (zom._state != ZombieState::SPAWN)	// 좀비가 깨어있지 않으므로 뷰리스트에 넣을일이 없다.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// 좀비가 깨어있어도 뷰리스트 범위 밖이면 그릴일이 없다.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				if (zom._state != ZombieState::SPAWN)	// 좀비가 깨어있지 않으므로 뷰리스트에 넣을일이 없다.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// 좀비가 깨어있어도 뷰리스트 범위 밖이면 그릴일이 없다.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			for (auto& zom : b_zombie1)
			{
				if (zom._state != ZombieState::SPAWN)	// 좀비가 깨어있지 않으므로 뷰리스트에 넣을일이 없다.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// 좀비가 깨어있어도 뷰리스트 범위 밖이면 그릴일이 없다.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				if (zom._state != ZombieState::SPAWN)	// 좀비가 깨어있지 않으므로 뷰리스트에 넣을일이 없다.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// 좀비가 깨어있어도 뷰리스트 범위 밖이면 그릴일이 없다.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				if (zom._state != ZombieState::SPAWN)	// 좀비가 깨어있지 않으므로 뷰리스트에 넣을일이 없다.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// 좀비가 깨어있어도 뷰리스트 범위 밖이면 그릴일이 없다.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		}

		//cout << "client_id [ " << client_id << "] x : " << cl.player->x << ", z : " << cl.player->z << "\n";

		cl.list_lock.lock();
		unordered_set<int> cl_z_list{ cl.zombie_list };
		cl.list_lock.unlock();

		// 이동 했을 때 주변에 좀비가 있을경우 뷰리스트에 넣는다.
		for (auto& zom : near_zombie_list)	// 가까운 좀비 리스트
		{
			if (0 == cl_z_list.count(zom))	// 근데 지금 클라의 좀비 리스트에 있지 않다면?
			{
				cl.list_lock.lock();
				cl.zombie_list.insert(zom);
				cl.list_lock.unlock();

				switch (cl.map_type)
				{
				case MapType::FIRST_PATH:
				{
					Send_viewlist_put_packet(cl._id, zom, cl.map_type, r_zombie1[zom].zombie->GetX(), r_zombie1[zom].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, r_zombie1[zom].zombie->_type);
					break;
				}
				case MapType::SECOND_PATH:
				{
					Send_viewlist_put_packet(cl._id, zom, cl.map_type, r_zombie2[zom].zombie->GetX(), r_zombie2[zom].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, r_zombie2[zom].zombie->_type);
					break;
				}
				case MapType::FINAL_PATH:
				{
					Send_viewlist_put_packet(cl._id, zom, cl.map_type, r_zombie3[zom].zombie->GetX(), r_zombie3[zom].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, r_zombie3[zom].zombie->_type);
					break;
				}
				case MapType::CHECK_POINT_ONE:
				{
					Send_viewlist_put_packet(cl._id, zom, cl.map_type, b_zombie1[zom].zombie->GetX(), b_zombie1[zom].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, b_zombie1[zom].zombie->_type);
					break;
				}
				case MapType::CHECK_POINT_TWO:
				{
					Send_viewlist_put_packet(cl._id, zom, cl.map_type, b_zombie2[zom].zombie->GetX(), b_zombie2[zom].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, b_zombie2[zom].zombie->_type);
					break;
				}
				case MapType::CHECK_POINT_FINAL:
				{
					Send_viewlist_put_packet(cl._id, zom, cl.map_type, b_zombie3[zom].zombie->GetX(), b_zombie3[zom].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, b_zombie3[zom].zombie->_type);
					break;
				}
				}
			}
		}

		// 이동했을 때 기본에 있었는데 가까운데에 없어졌다면
		for (auto& zom : cl_z_list)	// 전 리스트
		{
			if (0 == near_zombie_list.count(zom))	// 근데 지금 없다면?
			{
				cl.list_lock.lock();
				cl.zombie_list.erase(zom);
				cl.list_lock.unlock();

				Send_viewlist_remove_packet(cl._id, zom, cl.map_type);
			}
		}

		// 이동 시 주변에 문이 있을 경우 해당 클라에게 해당 사항 보내기

		cl.move_lock.lock();
		int curr_x = (int)cl.player->x;
		int curr_z = (int)cl.player->z;
		cl.move_lock.unlock();

		bool escape = false;

		if (curr_x != pre_x || curr_z != pre_z)
		{
			for (int i = curr_z - 2; i <= curr_z + 2; ++i)
			{
				for (int j = curr_x - 2; j <= curr_x + 2; ++j)
				{
					if (map.map[i][j] == (char)MazeWall::DOOR)
					{
						Send_search_packet(cl._id, j, i, ObjectType::DOOR);
						escape = true;
						break;
					}
				}
				if (escape)
					break;
			}

			ChangeMapType(cl);

			//if (escape == false)
			//{
			//	Send_fail_packet(cl._id, MsgType::SC_PLAYER_SEARCH_FAIL);
			//}

			switch (map_type)
			{
			case MapType::FIRST_PATH:
			{
				for (int i = 0; i < ROAD_ZOMBIE_NUM; ++i)
				{
					if (r_zombie1[i]._state == ZombieState::SLEEP)
						ChangeRoadZombieStateToSpawn(r_zombie1[i]);
				}
				break;
			}
			case MapType::SECOND_PATH:
			{
				for (int i = 0; i < ROAD_ZOMBIE_NUM; ++i)
				{
					if (r_zombie2[i]._state == ZombieState::SLEEP)
						ChangeRoadZombieStateToSpawn(r_zombie2[i]);
				}
				break;
			}
			case MapType::FINAL_PATH:
			{
				for (int i = 0; i < ROAD_ZOMBIE_NUM; ++i)
				{
					if (r_zombie3[i]._state == ZombieState::SLEEP)
						ChangeRoadZombieStateToSpawn(r_zombie3[i]);
				}
				break;
			}
			}
		}

		cl.idle_time = chrono::system_clock::now();
		break;
	}
	case (int)MsgType::CS_PLAYER_SELECT:
	{
		cs_select_packet* packet = reinterpret_cast<cs_select_packet*>(p);
		bool escape = false;
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state && ClientState::DEAD != other._state)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			if (other._type == packet->playertype) {
				Send_fail_packet(client_id, MsgType::SC_PLAYER_SELECT_FAIL);
				escape = true;
				break;
			}
		}
		if (escape == true) break;

		cl._type = packet->playertype;

		switch (cl._type)
		{
		case PlayerType::COMMANDER:
		{
			cl.player = new Commander;
			break;
		}
		case PlayerType::ENGINEER:
		{
			cl.player = new Engineer;
			break;
		}
		case PlayerType::MERCENARY:
		{
			cl.player = new Mercynary;
			break;
		}
		default:
		{
			cout << "플레이어 타입이 설정되지 않았습니다 \n";
		}
		}

		Send_select_packet(cl._id, cl._id);

		// 다른 플레이어에게 자신이 선택한 캐릭터 보내기
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			Send_select_packet(cl._id, other._id);
		}

		// 다른 플레이어가 선택한 거 자신에게 보내기
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state)
			{
				//other.state_lock.unlock();
				continue;
			}
			//other.state_lock.unlock();

			if (other._type == PlayerType::NONE)
				continue;

			Send_select_packet(other._id, cl._id);
		}
		break;
	}
	case (int)MsgType::CS_PLAYER_SPECIAL:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		if (cl._type == PlayerType::COMMANDER)
		{
			CommanderSpecialSkill(cl);
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			EngineerSpecialSkill(cl);
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			MercenarySpecialSkill(cl);
		}

		cl.idle_time = chrono::system_clock::now();
		break;
	}
	case (int)MsgType::CS_BARRICADE_REQUEST:
	{
		bool barricade_build = true;
		
		for (auto& client : g_clients)
		{
			//client.state_lock.lock();
			if (ClientState::INGAME != client._state)
			{
				Send_fail_packet(cl._id, MsgType::SC_GAME_START_FAIL);
				barricade_build = false;
				//client.state_lock.unlock();
				break;
			}
			//client.state_lock.unlock();
		}
		
		if (barricade_build && !cl.send_start_packet)
		{
			Send_barricade_packet(cl._id);
		}
		break;
	}
	case (int)MsgType::CS_GAME_START_REQUEST:
	{
		//cl.start_lock.lock();
		cl.send_start_packet = true;
		//cl.start_lock.unlock();

		bool game_start = true;

		
		for (auto& other : g_clients)
		{
			if (other._id == cl._id)
				continue;

			//other.state_lock.lock();
			if (ClientState::INGAME != other._state || other.send_start_packet == false)
			{
				game_start = false;
				//other.state_lock.unlock();
				break;
			}
			//other.state_lock.unlock();
		}
		

		if (game_start)
		{
			for (auto& client : g_clients)
			{
				//client.state_lock.lock();
				if (client._state != ClientState::INGAME)
				{
					//client.state_lock.unlock();
					continue;
				}
				//client.state_lock.unlock();
				Send_game_start_packet(client._id);
			}
		}
		else
			Send_fail_packet(cl._id, MsgType::SC_GAME_START_FAIL);

		break;
	}
	case (int)MsgType::CS_GAME_START:
	{
		cout << cl._id << " : 게임 시작 성공 \n";

		game_start = true;
		break;
	}
	case (int)MsgType::CS_SERVER_REQUEST:
	{
		sc_wait_packet packet;
		packet.size = sizeof(packet);
		packet.type = MsgType::SC_WAIT;
		packet.id = cl._id;
		cl.do_send(sizeof(packet), &packet);
		cl.idle_time = chrono::system_clock::now();
		break;
	}
	default:
	{
		cout << "잘못왔음 \n";
		break;
	}
	}
}

void Server::Send_zombie_spawn_packet(int c_id, int z_id, float x, float z, ZombieType type, int hp, float angle)
{
	sc_zombie_spawn_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_SPAWN;
	packet.x = x;
	packet.z = z;
	packet.id = (unsigned char)z_id;
	packet.zomtype = type;
	packet.map_type = map_type;
	packet.state = ZombieState::SPAWN;
	packet.hp = hp;
	packet.angle = angle;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

bool Server::NCDis_check(int c_id,  NPC& npc)
{
	if (CAMERA_WIDTH < abs(g_clients[c_id].player->x - npc.zombie->x)) return false;
	if (CAMERA_HEIGHT < abs(g_clients[c_id].player->z - npc.zombie->z)) return false;

	return true;	// NPC랑 CLIENT는 가까이 있으므로 zombie_list에 넣어야 한다.
}

float Server::ZombieSetAngle(NPC& npc, Client& cl)
{
	float x = cl.player->x - npc.zombie->GetX();
	float z = cl.player->z - npc.zombie->GetZ();

	float radian = atan2f(z, x);
	float degree = radian * 180 / 3.141592f;

	if (degree < 0)
		degree += 360;

	degree -= 90;
	degree *= -1.0f;

	return degree;
}

void Server::ChangeRoadZombieStateToSpawn(NPC& npc)
{
	bool spawn = false;
	for (auto& cl : g_clients)
	{
		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			continue;
		}
		//cl.state_lock.unlock();

		float DisZP = Distance(cl.player->x, cl.player->z, npc.zombie->GetX(), npc.zombie->GetZ());
		if (DisZP > 10.0f)
		{
			continue;
		}

		//npc.state_lock.lock();
		npc._state = ZombieState::SPAWN;
		//npc.state_lock.unlock();

		npc.zombie->angle = ZombieSetAngle(npc, cl);

		spawn = true;
		// 거리 이내에 있으면 스폰 Post큐 해서 ZombieSpawn으로 보내야하나?
		//cout << "zombie id : " << npc._id << "가 x : " << npc.zombie->x << ", z : " << npc.zombie->z << "에 스폰함 \n";

		break;
	}

	if (spawn)
	{
		for (auto& cl : g_clients)
		{
			//cl.state_lock.lock();
			if (cl._state != ClientState::INGAME)
			{
				//cl.state_lock.unlock();
				continue;
			}
			//cl.state_lock.unlock();

			//Send_zombie_spawn_packet(cl._id, npc._id, npc.zombie->GetX(), npc.zombie->GetZ(), npc.zombie->_type, npc.zombie->hp);
			Send_zombie_spawn_packet(cl._id, npc._id, npc.zombie->GetX(), npc.zombie->GetZ(), npc.zombie->_type, npc.zombie->hp, npc.zombie->angle);

			// 아직 viewlist를 만들지 않아서 해당 부분을 만들어야함
			// viewlist안에 들어와 있는 경우에만 spawn 시작을 알려야함
			// 아직 하지 않은 이유는 카메라크기에 대한 정의가 아직 안되어있으며, 어떻게 될지 모르기에 그냥 모두 스폰한걸 클라이언트에게 넘김
			// viewlist 안에 없으면 클라이언트에 보낼 이유가 없음
			
			if (NCDis_check(cl._id, npc) == false)	// 거리 멀면 보낼 이유가 없으므로... continue
				continue;


			cl.list_lock.lock();
			cl.zombie_list.insert(npc._id);
			cl.list_lock.unlock();

			Send_viewlist_put_packet(cl._id, npc._id, cl.map_type, npc.zombie->GetX(), npc.zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, npc.zombie->_type);
		}

		//this_thread::sleep_for(1ms);
		AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_MOVE, 667);
	}

}

void Server::ChangeZombieStateToSpawn(int spawn_id)
{
	switch (map_type)
	{
	case MapType::CHECK_POINT_ONE:
	{
		bool spawn = false;

		for (int i = spawn_id; i < spawn_id+3; ++i)
		{
			this_thread::sleep_for(1ms);
			if (i >= FIRST_CHECK_POINT_ZOMBIE_NUM)
			{
				spawn = false;
				break;
			}

			if (b_zombie1[i]._state == ZombieState::SLEEP)
			{
				spawn = true;
				
				b_zombie1[i]._state = ZombieState::SPAWN;

				float root_path[3] = { 999999, 999999, 999999 };

				for (auto& cl : g_clients)
				{
					if (cl._state != ClientState::INGAME) continue;
					
					root_path[cl._id] = Distance(cl.player->x, cl.player->z, b_zombie1[i].zombie->GetX(), b_zombie1[i].zombie->GetZ());
				}

				if(root_path[0] < root_path[1] && root_path[0] < root_path[2])
					b_zombie1[i].zombie->angle = ZombieSetAngle(b_zombie1[i], g_clients[0]);
				if (root_path[1] < root_path[0] && root_path[1] < root_path[2])
					b_zombie1[i].zombie->angle = ZombieSetAngle(b_zombie1[i], g_clients[1]);
				if (root_path[2] < root_path[1] && root_path[2] < root_path[0])
					b_zombie1[i].zombie->angle = ZombieSetAngle(b_zombie1[i], g_clients[2]);

				for (auto& cl : g_clients)
				{
					if (cl._state != ClientState::INGAME) continue;

					Send_zombie_spawn_packet(cl._id, i, b_zombie1[i].zombie->GetX(), b_zombie1[i].zombie->GetZ(), b_zombie1[i].zombie->_type, b_zombie1[i].zombie->hp, b_zombie1[i].zombie->angle);

					// 아직 viewlist를 만들지 않아서 해당 부분을 만들어야함
					// viewlist안에 들어와 있는 경우에만 spawn 시작을 알려야함
					// 아직 하지 않은 이유는 카메라크기에 대한 정의가 아직 안되어있으며, 어떻게 될지 모르기에 그냥 모두 스폰한걸 클라이언트에게 넘김
					// viewlist 안에 없으면 클라이언트에 보낼 이유가 없음
					if (NCDis_check(cl._id, b_zombie1[i]) == false)	// 거리 멀면 보낼 이유가 없으므로... continue
						continue;

					cl.list_lock.lock();
					cl.zombie_list.insert(i);
					cl.list_lock.unlock();

					Send_viewlist_put_packet(cl._id, i, map_type, b_zombie1[i].zombie->GetX(), b_zombie1[i].zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, b_zombie1[i].zombie->_type);
				}
				//cout << i << "번째 거점 좀비가 x : " << b_zombie1[i].zombie->GetX() << ", z : " << b_zombie1[i].zombie->GetZ() << "에 출현하였습니다 \n";

				AddTimer(i, EVENT_TYPE::EVENT_NPC_MOVE, 667);
			}
			else
			{
				cout << "이미 스폰되어 있거나 죽어 있습니다.(그럴 수 없는데?) \n";
			}
		}

		if (spawn) {
			AddTimer(spawn_id + 3, EVENT_TYPE::EVENT_NPC_SPAWN, 1000);
		}

		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		bool spawn = false;

		for (int i = spawn_id; i < spawn_id + 3; ++i)
		{
			this_thread::sleep_for(1ms);
			if (i >=TWO_CHECK_POINT_ZOMBIE_NUM)
			{
				spawn = false;
				break;
			}
			
			if (b_zombie2[i]._state == ZombieState::SLEEP)
			{
				spawn = true;

				b_zombie2[i]._state = ZombieState::SPAWN;

				float root_path[3] = { 999999, 999999, 999999 };

				for (auto& cl : g_clients)
				{
					if (cl._state != ClientState::INGAME) continue;

					root_path[cl._id] = Distance(cl.player->x, cl.player->z, b_zombie2[i].zombie->GetX(), b_zombie2[i].zombie->GetZ());
				}

				if (root_path[0] < root_path[1] && root_path[0] < root_path[2])
					b_zombie2[i].zombie->angle = ZombieSetAngle(b_zombie2[i], g_clients[0]);
				if (root_path[1] < root_path[0] && root_path[1] < root_path[2])
					b_zombie2[i].zombie->angle = ZombieSetAngle(b_zombie2[i], g_clients[1]);
				if (root_path[2] < root_path[1] && root_path[2] < root_path[0])
					b_zombie2[i].zombie->angle = ZombieSetAngle(b_zombie2[i], g_clients[2]);

				for (auto& cl : g_clients)
				{
					if (cl._state != ClientState::INGAME) continue;

					Send_zombie_spawn_packet(cl._id, i, b_zombie2[i].zombie->GetX(), b_zombie2[i].zombie->GetZ(), b_zombie2[i].zombie->_type, b_zombie2[i].zombie->hp, b_zombie2[i].zombie->angle);

					// 아직 viewlist를 만들지 않아서 해당 부분을 만들어야함
					// viewlist안에 들어와 있는 경우에만 spawn 시작을 알려야함
					// 아직 하지 않은 이유는 카메라크기에 대한 정의가 아직 안되어있으며, 어떻게 될지 모르기에 그냥 모두 스폰한걸 클라이언트에게 넘김
					// viewlist 안에 없으면 클라이언트에 보낼 이유가 없음

					if (NCDis_check(cl._id, b_zombie2[i]) == false)	// 거리 멀면 보낼 이유가 없으므로... continue
						continue;

					cl.list_lock.lock();
					cl.zombie_list.insert(i);
					cl.list_lock.unlock();

					Send_viewlist_put_packet(cl._id, i, map_type, b_zombie2[i].zombie->GetX(), b_zombie2[i].zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, b_zombie2[i].zombie->_type);
				}
				//cout << i << "번째 거점 좀비가 x : " << b_zombie2[i].zombie->GetX() << ", z : " << b_zombie2[i].zombie->GetZ() << "에 출현하였습니다 \n";
				AddTimer(i, EVENT_TYPE::EVENT_NPC_MOVE, 667);
			}
			else
			{
				cout << "이미 스폰되어 있거나 죽어 있습니다.(그럴 수 없는데?) \n";
			}
		}

		if (spawn) {
			AddTimer(spawn_id + 3, EVENT_TYPE::EVENT_NPC_SPAWN, 1000);
		}

		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		bool spawn = false;

		for (int i = spawn_id; i < spawn_id + 6; ++i)
		{
			if (i >= THREE_CHECK_POINT_ZOMBIE_NUM)
			{
				spawn = false;
				break;
			}

			if (b_zombie3[i]._state == ZombieState::SLEEP)
			{

				spawn = true;
				b_zombie3[i]._state = ZombieState::SPAWN;

				float root_path[3] = { 999999, 999999, 999999 };

				for (auto& cl : g_clients)
				{
					if (cl._state != ClientState::INGAME) continue;

					root_path[cl._id] = Distance(cl.player->x, cl.player->z, b_zombie3[i].zombie->GetX(), b_zombie3[i].zombie->GetZ());
				}

				if (root_path[0] < root_path[1] && root_path[0] < root_path[2])
					b_zombie3[i].zombie->angle = ZombieSetAngle(b_zombie3[i], g_clients[0]);
				if (root_path[1] < root_path[0] && root_path[1] < root_path[2])
					b_zombie3[i].zombie->angle = ZombieSetAngle(b_zombie3[i], g_clients[1]);
				if (root_path[2] < root_path[1] && root_path[2] < root_path[0])
					b_zombie3[i].zombie->angle = ZombieSetAngle(b_zombie3[i], g_clients[2]);

				for (auto& cl : g_clients)
				{
					if (cl._state != ClientState::INGAME) continue;

					Send_zombie_spawn_packet(cl._id, i, b_zombie3[i].zombie->GetX(), b_zombie3[i].zombie->GetZ(), b_zombie3[i].zombie->_type, b_zombie3[i].zombie->hp, b_zombie3[i].zombie->angle);

					// 아직 viewlist를 만들지 않아서 해당 부분을 만들어야함
					// viewlist안에 들어와 있는 경우에만 spawn 시작을 알려야함
					// 아직 하지 않은 이유는 카메라크기에 대한 정의가 아직 안되어있으며, 어떻게 될지 모르기에 그냥 모두 스폰한걸 클라이언트에게 넘김
					// viewlist 안에 없으면 클라이언트에 보낼 이유가 없음

					if (NCDis_check(cl._id, b_zombie3[i]) == false)	// 거리 멀면 보낼 이유가 없으므로... continue
						continue;

					cl.list_lock.lock();
					cl.zombie_list.insert(i);
					cl.list_lock.unlock();

					Send_viewlist_put_packet(cl._id, i, map_type, b_zombie3[i].zombie->GetX(), b_zombie3[i].zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, b_zombie3[i].zombie->_type);
				}
				//cout << i << "번째 거점 좀비가 x : " << b_zombie3[i].zombie->GetX() << ", z : " << b_zombie3[i].zombie->GetZ() << "에 출현하였습니다 \n";
				AddTimer(i, EVENT_TYPE::EVENT_NPC_MOVE, 667);
			}
			else
			{
				cout << "이미 스폰되어 있거나 죽어 있습니다.(그럴 수 없는데?) \n";
			}
		}

		if (spawn) {
			AddTimer(spawn_id + 6, EVENT_TYPE::EVENT_NPC_SPAWN, 1000);
		}

		break;
	}
	}
}

void Server::Send_zombie_move_packet(int c_id, int z_id, float x, float z, MapType m_type, float t_x, float t_z, float speed, Direction dir)
{
	sc_zombie_move_packet packet;
	packet.id = (char)z_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_MOVE;
	packet.x = x;
	packet.z = z;
	packet.map_type = m_type;
	packet.t_x = t_x;
	packet.t_z = t_z;
	packet.speed = speed;
	packet.dir = dir;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

void Server::Send_zombie_dead_packet(int c_id, int z_id, MapType m_type)
{
	sc_zombie_dead_packet packet;
	packet.id = z_id;
	packet.m_type = m_type;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_DEAD;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

void Server::ZombieDead(NPC& npc, MapType m_type)
{
	npc._state = ZombieState::DEAD;
	npc.zombie->astar.Delete();

	delete npc.zombie;
	npc.zombie = nullptr;
	
	//r_zombie1[z_id].state_lock.unlock();

	for (auto& cl : g_clients)
	{
		//cl.state_lock.lock();
		if (cl._state != ClientState::INGAME)
		{
			//cl.state_lock.unlock();
			continue;
		}
		//cl.state_lock.unlock();

		if (cl.zombie_list.count(npc._id) != 0)
		{
			cl.list_lock.lock();
			cl.zombie_list.erase(npc._id);
			cl.list_lock.unlock();

			Send_viewlist_remove_packet(cl._id, npc._id, map_type);
		}

		Send_zombie_dead_packet(cl._id, npc._id, m_type);
	}
}

bool Server::ZombieAttackRangeCheck(Direction dir, float att_range, float x, float z, float c_x, float c_z)
{
	switch (dir)
	{
	case Direction::UP:
	{
		if (x - att_range <= c_x && c_x <= x + att_range)
		{
			if (z <= c_z && c_z <= z + att_range)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;

		break;
	}
	case Direction::UP_RIGHT:
	{
		if ((((2.0f * x) - (sqrt(2) * att_range)) / 2) <= c_x && c_x < x)
		{
			float result1 = -c_x + x + z;
			float result2 = c_x - x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else if (x <= c_x && c_x <= (((2 * x) + (sqrt(2) * att_range)) / 2))
		{
			float result1 = -c_x + x + z;
			float result2 = -c_x + x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else
		{
			float result1 = c_x - x + z - ((float)sqrt(2) * att_range);
			float result2 = -c_x + x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}

		break;
	}
	case Direction::RIGHT:
	{
		if (x <= c_x && c_x <= x + att_range)
		{
			if (z - att_range <= c_z && c_z <= z + att_range)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;

		break;
	}
	case Direction::DOWN_RIGHT:
	{
		if ((((2 * x) - (sqrt(2) * att_range)) / 2) <= c_x && c_x < x)
		{
			float result1 = -c_x + x + z - ((float)sqrt(2) * att_range);
			float result2 = c_x - x + z;

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;	
		}
		else if (x <= c_x && c_x <= (((2 * x) + (sqrt(2) * att_range)) / 2))
		{
			float result1 = c_x - x + z - ((float)sqrt(2) * att_range);
			float result2 = c_x - x + z;

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else
		{
			float result1 = c_x - x + z - ((float)sqrt(2) * att_range);
			float result2 = -c_x + x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}

		break;
	}
	case Direction::DOWN:
	{
		if (x - att_range <= c_x && c_x <= x + att_range)
		{
			if (z - att_range <= c_z && c_z <= z)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;

		break;
	}
	case Direction::DOWN_LEFT:
	{
		if (x - (sqrt(2)*att_range) <= c_x && c_x < x - ((sqrt(2)*att_range) / 2))
		{
			float result1 = -c_x + x + z - ((float)sqrt(2) * att_range);
			float result2 = c_x - x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else if (x - ((sqrt(2) * att_range) / 2) <= c_x && c_x < x)
		{
			float result1 = -c_x + x + z - ((float)sqrt(2) * att_range);
			float result2 = -c_x + x + z;

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else
		{
			float result1 = c_x - x + z - ((float)sqrt(2) * att_range);
			float result2 = -c_x + x + z;

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}

		break;
	}
	case Direction::LEFT:
	{
		if (x - att_range <= c_x && c_x <= x)
		{
			if (z - att_range <= c_z && c_z <= z + att_range)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;

		break;
	}
	case Direction::UP_LEFT:
	{
		if (x - (sqrt(2) * att_range) <= c_x && c_x < x - ((sqrt(2) * att_range) / 2))
		{
			float result1 = -c_x + x + z - ((float)sqrt(2) * att_range);
			float result2 = c_x - x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else if (x - ((sqrt(2) * att_range) / 2) <= c_x && c_x < x)
		{
			float result1 = c_x - x + z;
			float result2 = c_x - x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}
		else
		{
			float result1 = c_x - x + z;
			float result2 = -c_x + x + z + ((float)sqrt(2) * att_range);

			if (result1 <= c_z && c_z <= result2)
				return true;
			else
				return false;
		}

		break;
	}
	default:
	{
		if (x - att_range < c_x && c_x < x + att_range)
		{
			if (z - (att_range / 2) < c_z && c_z < z + (att_range / 2))
				return true;
			else
				return false;
		}
		else
			return false;

		break;
	}
	}
}

void Server::Send_player_dead_packet(int c_id, int d_id)
{
	sc_player_dead_packet packet;
	packet.id = d_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_DEAD;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_player_info_packet(int c_id, int s_id, short hp)
{
	sc_player_hp_packet packet;
	packet.hp = hp;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_UPDATE_PLAYER_INFO;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_zombie_attack_packet(int c_id, int z_id, MapType m_type)
{
	sc_zombie_attack_packet packet;
	packet.id = z_id;
	packet.m_type = m_type;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_ATTACK;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

void Server::ZombiePlayerAttack(NPC& npc, MapType m_type)
{
	// 각 클라이언트에게 좀비가 공격 모션 하라고 전달
	for (auto& cl : g_clients)
	{
		if (cl._state != ClientState::INGAME)	continue;
		if (cl.map_type != map_type) continue;

		if (cl.zombie_list.count(npc._id) != 0)
			Send_zombie_attack_packet(cl._id, npc._id, m_type);
		else
			continue;

		// 공격 범위가 되는 지 확인
		bool attack_check = ZombieAttackRangeCheck(npc.zombie->zombie_dir, npc.zombie->attRange, npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);

		// 실패하면 다음 클라이언트 검사
		if (false == attack_check) continue;

		//cout << (int)npc._id << "의 공격으로 " << (int)cl._id << "의 hp가 " << (int)cl.player->hp << "에서";

		// 성공했으니 해당 클라의 hp를 npc의 데미지만큼 감소
		cl.player->hp -= npc.zombie->damage;

		//cout << (int)cl.player->hp << "로 감소하였습니다 \n";

		// 플레이어의 hp가 0이거나 아래가 되면 플레이어가 죽었다고 메시지 전송
		if (cl.player->hp <= 0)
		{
			cout << (int)cl._id << "가 죽었습니다 \n";

			for (auto& d_cl : g_clients)
			{
				if (d_cl._state != ClientState::INGAME) continue;

				Send_player_dead_packet(d_cl._id, cl._id);
			}

			// 플레이어 상태변화
			cl._state = ClientState::DEAD;

			int dead_player = 0;
			for (auto& d_cl : g_clients)
			{
				if (d_cl._state != ClientState::DEAD) break;

				dead_player++;
			}

			if (dead_player == 3)
			{
				for (auto& d_cl : g_clients)
				{
					if (d_cl._state != ClientState::DEAD) break;

					Send_fail_packet(d_cl._id, MsgType::SC_GAME_ALL_DEAD_END);
				}
			}
		}

		// 변한 플레이어의 hp 정보 모두에게 보내기
		for (auto& h_cl : g_clients)
		{
			if (h_cl._state != ClientState::INGAME)	continue;

			Send_player_info_packet(h_cl._id, cl._id, cl.player->hp);
			this_thread::sleep_for(1ms);
		}
	}

	npc.time_lock.lock();
	npc.attack_delay_time = chrono::system_clock::now() + npc.zombie->attack_timer;
	npc.time_lock.unlock();
}

void Server::ZombieAttack(int z_id)
{
	switch (map_type)
	{
	case MapType::FIRST_PATH:
	{
		ZombiePlayerAttack(r_zombie1[z_id], MapType::FIRST_PATH);
		break;
	}
	case MapType::SECOND_PATH:
	{
		ZombiePlayerAttack(r_zombie2[z_id], MapType::SECOND_PATH);

		break;
	}
	case MapType::FINAL_PATH:
	{
		ZombiePlayerAttack(r_zombie3[z_id], MapType::FINAL_PATH);

		break;
	}
	case MapType::CHECK_POINT_ONE:
	{
		ZombiePlayerAttack(b_zombie1[z_id], MapType::CHECK_POINT_ONE);

		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		ZombiePlayerAttack(b_zombie2[z_id], MapType::CHECK_POINT_TWO);

		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		ZombiePlayerAttack(b_zombie3[z_id], MapType::CHECK_POINT_FINAL);

		break;
	}
	}

	AddTimer(z_id, EVENT_TYPE::EVENT_NPC_MOVE, 800);
}

void Server::ZombieSend()
{
	for (auto& cl : g_clients)
	{
		if (cl._state != ClientState::INGAME)	continue;

		cl.size_lock.lock();
		cl.do_send(cl._zombie_prev_size, cl.zombie_send_buf);
		cl._zombie_prev_size = 0;
		ZeroMemory(cl.zombie_send_buf, sizeof(cl.zombie_send_buf));
		cl.size_lock.unlock();
	}
}

void Server::Send_player_idle_packet(int c_id, int s_id)
{
	sc_player_idle_packet packet;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_IDLE;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Work()
{
	while (1)
	{
		DWORD num_byte;
		ULONG_PTR iocp_key;
		WSAOVERLAPPED* p_over;
		HANDLE& t_iocp = _socket.ReturnHandle();

		BOOL ret = GetQueuedCompletionStatus(t_iocp, &num_byte, &iocp_key, &p_over, INFINITE);
		int c_id = static_cast<int>(iocp_key);
		Exp_Over* exp_over = reinterpret_cast<Exp_Over*>(p_over);
		if (FALSE == ret)
		{
			Disconnect(c_id);
			if (exp_over->_IOType == IOType::SEND)
			{
				delete exp_over;
				exp_over = nullptr;
			}
			continue;
		}

		switch (exp_over->_IOType)
		{
		case IOType::ACCEPT:
		{
			int new_id = Server::NewID();
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
			if (new_id == -1)
			{
				cout << "Max Player \n";
			}
			else
			{
				Client& cl = g_clients[new_id];

				cl._id = new_id;
				cl.prev_size = 0;
				cl._recv_over._IOType = IOType::RECV;
				cl._recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl._recv_over._net_buf);
				cl._recv_over._wsa_buf.len = sizeof(cl._recv_over._net_buf);
				ZeroMemory(&cl._recv_over._wsa_over, sizeof(cl._recv_over._wsa_over));
				cl.m_socket.s_socket = c_socket;

				_socket.OnlyCreatePort(c_socket, new_id);
				cl.do_recv();
			}

			int retval = _socket.Accept_Ex_IO(c_socket,*exp_over);
			}
			break;
		case IOType::RECV:
		{
			if (num_byte == 0)
			{
				Disconnect(c_id);
				continue;
			}
			Client& cl = g_clients[c_id];
			int remain_data = num_byte + cl.prev_size;
			unsigned char* packet_start = exp_over->_net_buf;
			int packet_size = ((int)packet_start[1] * 256 ) + (int)packet_start[0];

			while (packet_size <= remain_data)
			{
				ProcessPacket(c_id, packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0)
					packet_size = ((int)packet_start[1] * 256) + (int)packet_start[0];
				else
					break;
			}

			if (remain_data > 0)
			{
				cl.prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}
			cl.do_recv();
		}
			break;
		case IOType::SEND:
		{
			if (num_byte != exp_over->_wsa_buf.len) {
				Disconnect(c_id);
			}
			delete exp_over;
			exp_over = nullptr;
		}
			break;
		case IOType::NPC_SPAWN:
		{
			ChangeZombieStateToSpawn(c_id);
			delete exp_over;
			exp_over = nullptr;
		}
			break;
		case IOType::NPC_MOVE:
		{
			ZombieMove(c_id);
			delete exp_over;
			exp_over = nullptr;
		}
			break;
		case IOType::NPC_ATTACK:
		{
			ZombieAttack(c_id);

			delete exp_over;
			exp_over = nullptr;
		}
			break;
		case IOType::NPC_SEND:
		{
			if (g_clients[c_id]._zombie_prev_size == 0)
			{
				delete exp_over;
				exp_over = nullptr;

				AddTimer(c_id, EVENT_TYPE::EVENT_NPC_SEND, 100);
				break;
			}

			ZombieSend();
			delete exp_over;
			exp_over = nullptr;

			AddTimer(c_id, EVENT_TYPE::EVENT_NPC_SEND, 100);
		}
			break;
		case IOType::NPC_DEAD:
		{
			switch (map_type)
			{
			case MapType::FIRST_PATH:
			{
				if (r_zombie1[c_id]._state != ZombieState::SPAWN) break;

				ZombieDead(r_zombie1[c_id], MapType::FIRST_PATH);
				break;
			}
			case MapType::SECOND_PATH:
			{
				if (r_zombie2[c_id]._state != ZombieState::SPAWN) break;

				ZombieDead(r_zombie2[c_id], MapType::SECOND_PATH);
				break;
			}
			case MapType::FINAL_PATH:
			{
				if (r_zombie3[c_id]._state != ZombieState::SPAWN) break;

				ZombieDead(r_zombie3[c_id], MapType::FINAL_PATH);
				break;
			}
			case MapType::CHECK_POINT_ONE:
			{
				if (b_zombie1[c_id]._state != ZombieState::SPAWN) break;

				ZombieDead(b_zombie1[c_id], MapType::CHECK_POINT_ONE);
				break;
			}
			case MapType::CHECK_POINT_TWO:
			{
				if (b_zombie2[c_id]._state != ZombieState::SPAWN) break;

				ZombieDead(b_zombie2[c_id], MapType::CHECK_POINT_TWO);
				break;
			}
			case MapType::CHECK_POINT_FINAL:
			{
				if (b_zombie3[c_id]._state != ZombieState::SPAWN) break;

				ZombieDead(b_zombie3[c_id], MapType::CHECK_POINT_FINAL);
				break;
			}
			}

			delete exp_over;
			exp_over = nullptr;
		}
			break;
		case IOType::DOOR_OPEN:
		{
			int erase_door_number = 0;

			for (const auto& obj : map.map_info.Door)
			{
				if (obj.dir == DIR::WIDTH)
				{
					if (obj.row - (int)obj.size_x < g_clients[c_id].player->x && g_clients[c_id].player->x < obj.row + (int)obj.size_x)
					{
						if ((obj.col + (int)obj.size_z - 1 < g_clients[c_id].player->z && g_clients[c_id].player->z < obj.col + (int)obj.size_z + 2) || (obj.col - (int)obj.size_z - 2 < g_clients[c_id].player->z && g_clients[c_id].player->z < obj.col - (int)obj.size_z + 1))
						{
							for (int i = obj.col - (int)obj.size_z; i <= obj.col + (int)obj.size_z; ++i)
								for (int j = obj.row - (int)obj.size_x; j <= obj.row + (int)obj.size_x; ++j)
									map.map[i][j] = (char)MazeWall::ROAD;

							for (auto& cl : g_clients)
							{
								if (cl._state != ClientState::INGAME)	continue;

								Send_door_open_packet(cl._id, c_id, obj.row, obj.col, (int)obj.size_x, (int)obj.size_z);
							}

							map_lock.lock();
							if (door_num == obj.num)
							{
								map.map_info.Door.erase(map.map_info.Door.begin() + erase_door_number);
								door_num++;
							}
							map_lock.unlock();
							break;
						}
					}
				}
				else
				{
					if (obj.col - obj.size_z < g_clients[c_id].player->z && g_clients[c_id].player->z < obj.col + obj.size_z)
					{
						if ((obj.row - obj.size_x + 1.0f > g_clients[c_id].player->x && g_clients[c_id].player->x > obj.row - obj.size_x - 2.0f) || (obj.row + obj.size_x + 2.0f > g_clients[c_id].player->x && g_clients[c_id].player->x > obj.row + obj.size_x - 1.0f))
						{
							for (int i = obj.col - (int)obj.size_z; i <= obj.col + (int)obj.size_z; ++i)
								for (int j = obj.row - (int)obj.size_x; j <= obj.row + (int)obj.size_x; ++j)
									map.map[i][j] = (char)MazeWall::ROAD;

							for (auto& cl : g_clients)
							{
								if (cl._state != ClientState::INGAME)	continue;

								Send_door_open_packet(cl._id, c_id, obj.row, obj.col, (int)obj.size_x, (int)obj.size_z);
							}

							map_lock.lock();
							if (door_num == obj.num)
							{
								map.map_info.Door.erase(map.map_info.Door.begin() + erase_door_number);
								door_num++;
							}
							map_lock.unlock();
							break;
						}
					}
				}

				erase_door_number++;
			}

			delete exp_over;
			exp_over = nullptr;
		}
			break;
		default:
			cout << "이건 아무것도 아님 \n";
		}
	}
}

void Server::Send_zombie_search_packet(int c_id, int s_id, int z_id, MapType m_type)
{
	sc_zombie_search_packet packet;
	packet.m_type = m_type;
	packet.p_id = s_id;
	packet.z_id = z_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_SEARCH;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

void Server::Send_zombie_arrive_packet(int c_id, int z_id, MapType m_type, Direction dir)
{
	sc_zombie_arrive_packet packet;
	packet.id = z_id;
	packet.map_type = m_type;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_ARRIVE;
	packet.dir = dir;
	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	g_clients[c_id]._zombie_prev_size += packet.size;
	g_clients[c_id].size_lock.unlock();
}

void Server::SearchZombieAstar(int col, int row, Client& cl, NPC& npc)
{
	if (cl._state != ClientState::INGAME)
	{
		npc._state = ZombieState::SLEEP;

		return;
	}

	cl.move_lock.lock();
	int player_x = (int)g_clients[cl._id].player->x;
	int player_z = (int)g_clients[cl._id].player->z;
	cl.move_lock.unlock();

	stack<AS_Node*> temp;

	temp = npc.zombie->astar.AstartSearch(col, row, player_x, player_z);

	npc.zombie->z_move_lock.lock();
	npc.zombie->root = temp;
	npc.zombie->z_move_lock.unlock();
}

void Server::ZombieAstarMove(NPC& npc, MapType m_type, iPos start_pos, iPos end_pos)
{
	// 좀비가 SPAWN 상태가 아니라면 움직일 수 없으니 돌아가라
	if (npc._state != ZombieState::SPAWN)
	{
		return;
	}

	// 좀비가 다른 행동을 하고 Move로 온시간체크

	// 좀비 최초 위치에 다른 AS_MAP 설정
	if (npc.map_check == false)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(start_pos.x);
		npc.zombie->astar.Set_Start_Y(start_pos.z);
		npc.zombie->astar.Set_Map_Height(end_pos.z - start_pos.z);
		npc.zombie->astar.Set_Map_Width(end_pos.x - start_pos.x);
		npc.zombie->astar.Map(as_map);
		npc.map_check = true;
		npc.zombie->z_move_lock.unlock();
	}

	// Astar 알고리즘을 돌리지 않았다면 npc의 Astar 알고리즘 돌려서 스택 집어 넣기
	if (npc.astar_check == false)
	{
		float root_path[3] = { 999999, 999999, 999999 };

		// 각 클라이언트와의 거리 구하기
		for (auto& cl : g_clients)
		{
			if (cl._state != ClientState::INGAME) continue;

			if (cl.map_type != m_type) continue;

			root_path[cl._id] = Distance(npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);
		}

		// npc의 의미상 정수 좌표(?) 구하기
		int row = (int)npc.zombie->GetZ();
		int col = (int)npc.zombie->GetX();

		float row_result = npc.zombie->GetZ() - row;
		float col_result = npc.zombie->GetX() - col;

		if (row_result > 0.5f)
			row += 1;
		if (col_result > 0.5f)
			col += 1;

		// 거리가 가장 가까운 클라이언트를 목적지로 하는 Astar 구하기 
		if (root_path[0] < root_path[1] && root_path[0] < root_path[2])
		{
			if (root_path[0] >= 1.0f)
			{
				SearchZombieAstar(col, row, g_clients[0], npc);
				if (npc.zombie->root.size() == 0)
				{
					npc.astar_check = false;
					npc.zombie->astar.Delete();
				}
				else
					npc.astar_check = true;
			}
			else
			{
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}
		else if (root_path[1] < root_path[0] && root_path[1] < root_path[2])
		{
			if (root_path[1] >= 1.0f)
			{
				SearchZombieAstar(col, row, g_clients[1], npc);
				if (npc.zombie->root.size() == 0)
				{
					npc.astar_check = false;
					npc.zombie->astar.Delete();
				}
				else
					npc.astar_check = true;
			}
			else
			{
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}
		else if (root_path[2] < root_path[1] && root_path[2] < root_path[0])
		{
			if (root_path[2] >= 1.0f)
			{
				SearchZombieAstar(col, row, g_clients[2], npc);
				if (npc.zombie->root.size() == 0)
				{
					npc.astar_check = false;
					npc.zombie->astar.Delete();
				}
				else
					npc.astar_check = true;
			}
			else
			{
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}
		else
		{
			cout << "맵 영역에 아무도 없으므로 일단 수면 \n";
			npc._state = ZombieState::SLEEP;
			return;
		}
	}

	if (npc.zombie->root.size() == 1)
	{
		npc.astar_check = false;
		npc.zombie->astar.Delete();
	}

	if (npc.astar_check)
	{
		// 스피드는 좀비의 스피드
		z_speed = npc.zombie->speed / 10;

		unordered_set <int> prev_zombie_list;

		for (auto& cl : g_clients)
		{
			if (cl._state != ClientState::INGAME) continue;

			if (NCDis_check(cl._id, npc))
				prev_zombie_list.insert(cl._id);
		}

		MoveResult move_check = MoveResult::FAIL;

		float pre_x = npc.zombie->GetX();
		float pre_z = npc.zombie->GetZ();

		// Astar가 비어 있는다면 도착한 것이 되므로 else , 하지 않았다면 true 이므로 이동
		if (npc.zombie->root.empty() == false)
		{
			// 좀비가 Astar 대로 이동하는데 이동하는 지역에 따라 충돌을 했는지 실패했는지 성공했는지 파악
			move_check = npc.zombie->Move(z_speed, map);

			// 충돌하거나 실패했다면 다시 Astar 알고리즘을 돌려야함
			if (move_check != MoveResult::MOVE)
			{
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}
		else
		{
			npc.astar_check = false;
			npc.zombie->astar.Delete();
		}

		// Astar대로 이동에 성공
		if (move_check == MoveResult::MOVE)
		{
			float root_path[3] = { 999999, 999999, 999999 };

			// 각 클라이언트에게 어느 좀비가 이동했는지 보내기
			for (auto& cl : g_clients)
			{
				if (cl._state != ClientState::INGAME) continue;

				if (cl.map_type != m_type) continue;

				// 플레이어와 좀비간의 거리구하기
				root_path[cl._id] = Distance(npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);

				if (NCDis_check(cl._id, npc) == true) // 뷰리스트에 들어올만한 거리를 만족한다면
				{
					if (prev_zombie_list.count(cl._id) == 0)	// 근데 그전 뷰리스트에 없던 좀비라면? 추가!
					{
						cl.list_lock.lock();
						cl.zombie_list.insert(npc._id);
						cl.list_lock.unlock();

						Send_viewlist_put_packet(cl._id, npc._id, m_type, npc.zombie->GetX(), npc.zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, npc.zombie->_type);
					}

					float m_speed = npc.zombie->speed / 10;
					float t_x = 0, t_z = 0;

					switch (npc.zombie->zombie_dir)
					{
					case Direction::UP:
					{
						t_z = 1;
						break;
					}
					case Direction::UP_LEFT:
					{
						t_x = -0.7f;
						t_z = 0.7f;
						m_speed *= 0.7f;
						break;
					}
					case Direction::UP_RIGHT:
					{
						t_x = 0.7f;
						t_z = 0.7f;
						m_speed *= 0.7f;
						break;
					}
					case Direction::RIGHT:
					{
						t_x = 1;
						break;
					}
					case Direction::LEFT:
					{
						t_x = -1;
						break;
					}
					case Direction::DOWN:
					{
						t_z = -1;
						break;
					}
					case Direction::DOWN_LEFT:
					{
						t_x = -0.7f;
						t_z = -0.7f;
						m_speed *= 0.7f;
						break;
					}
					case Direction::DOWN_RIGHT:
					{
						t_x = 0.7f;
						t_z = -0.7f;
						m_speed *= 0.7f;
						break;
					}
					}

					//m_speed *= 100;

					// 좀비 이동 방향, 이동 전 위치, 이동 속도 등 보내기
					Send_zombie_move_packet(cl._id, npc._id, pre_x, pre_z, m_type, t_x, t_z, m_speed, npc.zombie->zombie_dir);

					// 이동에 성공했고, 서치 범위 내 플레이어가 접근했다면 다시 A*를 돌리고, 더이상 서치하지 않기
					if (npc.search_check == false)
					{
						//플레이어와의 거리가 서치범위 내에 접근했다면 좀비는 플레이어를 향해 한번 더 A* 돌려 접근하게 됨
						if (root_path[cl._id] <= npc.zombie->searchRange)
						{
							//npc.search_lock.lock();
							npc.astar_check = false;
							npc.zombie->astar.Delete();
							npc.search_check = true;
							//npc.search_lock.unlock();

							// 서치 범위내에 들어왔으므로 찾았다고 보내줌
							for (auto& s_cl : g_clients)
							{
								if (s_cl._state != ClientState::INGAME) continue;

								if (NCDis_check(s_cl._id, npc))
									Send_zombie_search_packet(s_cl._id, cl._id, npc._id, m_type);
							}
						}
					}
				}
				else
				{
					if (prev_zombie_list.count(cl._id) != 0)		// 지금 뷰리스트에 있지 않음에도 전에 뷰리스트에 있단건 뷰리스트 밖으로 빠져나간 것이므로 삭제
					{
						cl.list_lock.lock();
						cl.zombie_list.erase(npc._id);
						cl.list_lock.unlock();

						Send_viewlist_remove_packet(cl._id, npc._id, map_type);
					}
				}
			}
		}
	}
	else
	{
		float root_path[3] = { 999999, 999999, 999999 };

		// 각 클라이언트와의 거리 구하기
		for (auto& cl : g_clients)
		{
			if (cl._state != ClientState::INGAME) continue;

			if (cl.map_type != m_type) continue;
			if (NCDis_check(cl._id, npc) == false) continue;
			root_path[cl._id] = Distance(npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);
		}

		if (root_path[0] < root_path[1] && root_path[0] < root_path[2])
		{
			Direction dir = npc.zombie->RootDir(npc.zombie->GetX(), npc.zombie->GetZ(), g_clients[0].player->x, g_clients[0].player->z);

			npc.zombie->zombie_dir = dir;
			
			Send_zombie_arrive_packet(g_clients[0]._id, npc._id, map_type, npc.zombie->zombie_dir);
		}
		else if (root_path[1] < root_path[0] && root_path[1] < root_path[2])
		{
			Direction dir = npc.zombie->RootDir(npc.zombie->GetX(), npc.zombie->GetZ(), g_clients[1].player->x, g_clients[1].player->z);

			npc.zombie->zombie_dir = dir;
			Send_zombie_arrive_packet(g_clients[1]._id, npc._id, map_type, npc.zombie->zombie_dir);
		}
		else if (root_path[2] < root_path[1] && root_path[2] < root_path[0])
		{
			Direction dir = npc.zombie->RootDir(npc.zombie->GetX(), npc.zombie->GetZ(), g_clients[2].player->x, g_clients[2].player->z);

			npc.zombie->zombie_dir = dir;
			Send_zombie_arrive_packet(g_clients[2]._id, npc._id, map_type, npc.zombie->zombie_dir);
		}
	}

	// 이동에 성공했으면, 주변에 플레이어와 접근하여 또는 발견하여 플레이어를 충분히 공격할 수 있는 거리가 되었는지 확인
	if (npc.attack_delay_time < chrono::system_clock::now())
	{
		for (int i = 0; i < 3; ++i)
		{
			if (g_clients[i]._state != ClientState::INGAME) continue;
			if (g_clients[i].map_type != map_type) continue;

			// 거리가 좀비의 공격범위 보다 좁다면?
			if (Distance(g_clients[i].player->x, g_clients[i].player->z, npc.zombie->GetX(), npc.zombie->GetZ()) <= npc.zombie->attRange)
			{
				AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_ATTACK, 1);
				return;
			}
		}

		npc.attack_delay_time += 10ms;
	}

	// 30fps에 맞게 1초에 10번 이동
	AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_MOVE, 100);
}

void Server::ZombieMove(int z_id)
{
	switch (map_type)
	{
	case MapType::FIRST_PATH:
	{
		iPos temp = { One_Road_Pos3.x + 23 , One_Base_Pos.z };
		ZombieAstarMove(r_zombie1[z_id], MapType::FIRST_PATH, One_Road_Pos, temp);

		break;
	}
	case MapType::SECOND_PATH:
	{
		iPos temp = { TWO_Base_Pos.x-1, Two_Road_Pos.z+ ROAD_SIZE };
		ZombieAstarMove(r_zombie2[z_id], MapType::SECOND_PATH, Two_Road_Pos, temp);

		break;
	}
	case MapType::FINAL_PATH:
	{
		iPos temp = { THREE_Base_Pos.x, Three_Road_Pos.z + ROAD_SIZE };
		ZombieAstarMove(r_zombie3[z_id], MapType::FINAL_PATH, Three_Road_Pos, temp);

		break;
	}
	case MapType::CHECK_POINT_ONE:
	{
		ZombieAstarMove(b_zombie1[z_id], MapType::CHECK_POINT_ONE, One_Base_Pos, One_Base_End_Pos);

		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		ZombieAstarMove(b_zombie2[z_id], MapType::CHECK_POINT_TWO, TWO_Base_Pos, TWO_Base_End_Pos);

		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		ZombieAstarMove(b_zombie3[z_id], MapType::CHECK_POINT_FINAL, THREE_Base_Pos, THREE_Base_End_Pos2);

		break;
	}
	}
}

void Server::AddTimer(int z_id, EVENT_TYPE type, int duration)
{
	timer_lock.lock();
	timer_event te{ z_id, chrono::high_resolution_clock::now() + chrono::milliseconds(duration), type, 0 };
	timer_queue.push(te);
	timer_lock.unlock();
}

void Server::Do_Timer()
{
	while (true)
	{
		this_thread::sleep_for(1ms);
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
			timer_event te = timer_queue.top();
			timer_queue.pop();
			timer_lock.unlock();

			switch (te.ev)
			{
			case EVENT_TYPE::EVENT_DOOR_OPEN:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::DOOR_OPEN;
				HANDLE& t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.obj_id, &over->_wsa_over);
			}
				break;
			case EVENT_TYPE::EVENT_NPC_MOVE:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::NPC_MOVE; 
				HANDLE& t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.obj_id, &over->_wsa_over);
			}
				break;
			case EVENT_TYPE::EVENT_NPC_ATTACK:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::NPC_ATTACK;
				HANDLE& t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.obj_id, &over->_wsa_over);
			}
				break;
			case EVENT_TYPE::EVENT_NPC_SPAWN:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::NPC_SPAWN;
				HANDLE& t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.obj_id, &over->_wsa_over);
			}
				break;
			case EVENT_TYPE::EVENT_NPC_SEND:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::NPC_SEND;
				HANDLE& t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.obj_id, &over->_wsa_over);
			}
				break;
			case EVENT_TYPE::EVENT_NPC_DEAD:
			{
				Exp_Over* over = new Exp_Over;
				over->_IOType = IOType::NPC_DEAD;
				HANDLE& t_iocp = _socket.ReturnHandle();
				PostQueuedCompletionStatus(t_iocp, 1, te.obj_id, &over->_wsa_over);
			}
				break;
			}
		}
	}
}

void Server::Update()
{
	srand(time(NULL));

	Exp_Over p_over;
	_socket.Accept_Ex(p_over);

	Initialize();

	thread Timer_thread{ Do_Timer };
	
	for (int i = 0; i < 6; ++i) {
		worker_threads.emplace_back(Work);
	}

	Timer_thread.join();
	for (auto& th : worker_threads) {
		cout << "Join \n";
		th.join();
	}
}