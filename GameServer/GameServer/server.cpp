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

chrono::milliseconds Server::sec;
float Server::s_speed;
float Server::z_speed;

bool Server::zombie_send;
int Server::door_num;
bool Server::game_start;
int Server::remain_zombie_num;

char Server::select_type;

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
	 //���� �ʱ�ȭ ���� ��... �̸� ����� ���ϵ� ���⼭ �ҷ�����?
}

Server::~Server()
{
}

void Server::Initialize()
{
	game_timer.Start();

	select_type = 0;

	zombie_send = false;
	game_start = false;
	remain_zombie_num = 0;

	// �� ������ �о �浹üũ�� �� ���� ����
	map.ReadMapFile();
	map.Initialize();

	// ������ �̵� �˰����� A* �˰����� ����ϱ� ���� ���� �°� �ʱ�ȭ
	// �ٸ�����Ʈ�� �ʿ��ϱ� ������ �ռ� �ٸ�����Ʈ�� �����ϰ� �ʱ�ȭ�� ������
	as_map.InitMap(map);

	// ���� ���� ��ġ �ʱ�ȭ
	PlaceZombie(MapType::FIRST_PATH);
	PlaceZombie(MapType::SECOND_PATH);
	PlaceZombie(MapType::FINAL_PATH);
	PlaceZombie(MapType::CHECK_POINT_ONE);
	PlaceZombie(MapType::CHECK_POINT_TWO);
	PlaceZombie(MapType::CHECK_POINT_FINAL);

	door_num = 0;

	iPos temp = { One_Road_Pos3.x + 23 , One_Base_Pos.z };

	// ���� ���� ��ġ�� �ٸ� AS_MAP ����
	for (auto& npc : r_zombie1)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(One_Road_Pos.x);
		npc.zombie->astar.Set_Start_Y(One_Road_Pos.z);
		npc.zombie->astar.Set_Map_Height(temp.z - One_Road_Pos.z);
		npc.zombie->astar.Set_Map_Width(temp.x - One_Road_Pos.x);
		npc.zombie->astar.Map(as_map);
		//npc.zombie->astar.Init();
		npc.zombie->z_move_lock.unlock();
	}

	temp.x = TWO_Base_Pos.x - 1;
	temp.z = Two_Road_Pos.z + ROAD_SIZE;

	for (auto& npc : r_zombie2)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(Two_Road_Pos.x);
		npc.zombie->astar.Set_Start_Y(Two_Road_Pos.z);
		npc.zombie->astar.Set_Map_Height(temp.z - Two_Road_Pos.z);
		npc.zombie->astar.Set_Map_Width(temp.x - Two_Road_Pos.x);
		npc.zombie->astar.Map(as_map);
		//npc.zombie->astar.Init();
		npc.zombie->z_move_lock.unlock();
	}

	temp.x = THREE_Base_Pos.x;
	temp.z = Three_Road_Pos.z + ROAD_SIZE;

	for (auto& npc : r_zombie3)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(Three_Road_Pos.x);
		npc.zombie->astar.Set_Start_Y(Three_Road_Pos.z);
		npc.zombie->astar.Set_Map_Height(temp.z - Three_Road_Pos.z);
		npc.zombie->astar.Set_Map_Width(temp.x - Three_Road_Pos.x);
		npc.zombie->astar.Map(as_map);
		//npc.zombie->astar.Init();
		npc.zombie->z_move_lock.unlock();
	}

	for (auto& npc : b_zombie1)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(One_Base_Pos.x);
		npc.zombie->astar.Set_Start_Y(One_Base_Pos.z);
		npc.zombie->astar.Set_Map_Height(One_Base_End_Pos.z - One_Base_Pos.z);
		npc.zombie->astar.Set_Map_Width(One_Base_End_Pos.x - One_Base_Pos.x);
		npc.zombie->astar.Map(as_map);
		//npc.zombie->astar.Init();
		npc.zombie->z_move_lock.unlock();
	}

	for (auto& npc : b_zombie2)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(TWO_Base_Pos.x);
		npc.zombie->astar.Set_Start_Y(TWO_Base_Pos.z);
		npc.zombie->astar.Set_Map_Height(TWO_Base_End_Pos.z - TWO_Base_Pos.z);
		npc.zombie->astar.Set_Map_Width(TWO_Base_End_Pos.x - TWO_Base_Pos.x);
		npc.zombie->astar.Map(as_map);
		//npc.zombie->astar.Init();
		npc.zombie->z_move_lock.unlock();
	}

	for (auto& npc : b_zombie3)
	{
		npc.zombie->z_move_lock.lock();
		npc.zombie->astar.Set_Start_X(THREE_Base_Pos.x);
		npc.zombie->astar.Set_Start_Y(THREE_Base_Pos.z);
		npc.zombie->astar.Set_Map_Height(THREE_Base_End_Pos2.z - THREE_Base_Pos.z);
		npc.zombie->astar.Set_Map_Width(THREE_Base_End_Pos2.x - THREE_Base_Pos.x);
		npc.zombie->astar.Map(as_map);
		//npc.zombie->astar.Init();
		npc.zombie->z_move_lock.unlock();
	}
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
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_select_packet(int c_id, int s_id, char s_type)
{
	sc_player_select_packet packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_SELECT;
	packet.playertype = g_clients[c_id]._type;
	packet.hp = g_clients[c_id].player->hp;
	packet.maxhp = g_clients[c_id].player->maxhp;
	packet.x = g_clients[c_id].player->x;
	packet.z = g_clients[c_id].player->z;
	packet.bullet = 30;
	packet.speed = g_clients[c_id].player->speed;
	packet.select_type = s_type;
	g_clients[s_id].do_send(sizeof(packet), &packet);
}

void Server::Send_player_move_packet(int c_id, int s_id, float x, float z, float t_x, float t_z, float speed, float rotation, bool input)
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
	packet.rotation_angle = rotation;
	packet.in_input = input;
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
	packet.type = MsgType::SC_BARRICADE_SEND;
	memcpy(packet.one_base, map.one_base, sizeof(map.one_base));
	memcpy(packet.two_base, map.two_base, sizeof(map.two_base));
	memcpy(packet.three_base, map.three_base, sizeof(map.three_base));
	memcpy(packet.three_base2, map.three_base2, sizeof(map.three_base2));
	packet.size = sizeof(packet);

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

void Server::Send_zombie_number_packet(int c_id, int z_num)
{
	sc_zombie_num_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_NUMBER;
	packet.zombie_num = z_num;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

bool Server::MapCheck(MapType map_type)
{
	for (auto& cl : g_clients)
	{
		if (cl._state != ClientState::INGAME)
		{
			continue;
		}

		if (cl.map_type != map_type)
		{
			return false;
		}
	}

	return true;
}

void Server::InitZombie(NPC& npc, int &i, float &PosX, float &PosZ)
{
	int random_zombie_type = rand() % 3;
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
	//case 3:
	//	npc.zombie = new DogZombie;
	//	npc._id = i;
	//	npc.zombie->SetX(PosX);
	//	npc.zombie->SetZ(PosZ);
	//	break;
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

	if ((cl.map_type != MapType::FIRST_PATH) && ((One_Road_Pos.x <= cl.player->x && cl.player->x <= One_Road_Pos3.x) && (One_Road_Pos.z <= cl.player->z && cl.player->z <= One_Base_Pos.z - 1)))
	{
		cl.zombie_list.clear();
		change = MapType::FIRST_PATH;
		cl.map_type = MapType::FIRST_PATH;
		Send_zombie_number_packet(cl._id, ROAD_ZOMBIE_NUM);
		if (MapCheck(MapType::FIRST_PATH))
		{
			remain_zombie_num = ROAD_ZOMBIE_NUM;
		}
	}
	else if ((cl.map_type != MapType::CHECK_POINT_ONE) && ((One_Base_Pos.x <= cl.player->x && cl.player->x <= One_Base_End_Pos.x) && (One_Base_Pos.z <= cl.player->z && cl.player->z <= One_Base_End_Pos.z)))
	{
		cl.zombie_list.clear();
		change = MapType::CHECK_POINT_ONE;
		cl.map_type = MapType::CHECK_POINT_ONE;
		Send_zombie_number_packet(cl._id, FIRST_CHECK_POINT_ZOMBIE_NUM);
		if (MapCheck(MapType::FIRST_PATH))
		{
			remain_zombie_num = FIRST_CHECK_POINT_ZOMBIE_NUM;
		}
	}
	else if ((cl.map_type != MapType::CHECK_POINT_TWO) && ((TWO_Base_Pos.x <= cl.player->x && cl.player->x <= TWO_Base_End_Pos.x) && (TWO_Base_Pos.z <= cl.player->z && cl.player->z <= TWO_Base_End_Pos.z)))
	{
		cl.zombie_list.clear();
		change = MapType::CHECK_POINT_TWO;
		cl.map_type = MapType::CHECK_POINT_TWO;
		Send_zombie_number_packet(cl._id, TWO_CHECK_POINT_ZOMBIE_NUM);
		if (MapCheck(MapType::FIRST_PATH))
		{
			remain_zombie_num = TWO_CHECK_POINT_ZOMBIE_NUM;
		}
	}
	else if ((cl.map_type != MapType::CHECK_POINT_FINAL) && ((THREE_Base_Pos.x <= cl.player->x && cl.player->x <= THREE_Base_End_Pos2.x) && (THREE_Base_Pos.z <= cl.player->z && cl.player->z <= THREE_Base_End_Pos.z)))
	{
		cl.zombie_list.clear();
		change = MapType::CHECK_POINT_FINAL;
		cl.map_type = MapType::CHECK_POINT_FINAL;
		Send_zombie_number_packet(cl._id, THREE_CHECK_POINT_ZOMBIE_NUM);
		if (MapCheck(MapType::FIRST_PATH))
		{
			remain_zombie_num = THREE_CHECK_POINT_ZOMBIE_NUM;
		}
	}
	else if((cl.map_type != MapType::SECOND_PATH) && ((Two_Road_Pos.x <= cl.player->x&& cl.player->x <= TWO_Base_Pos.x) && (Two_Road_Pos.z <= cl.player->z&& cl.player->z <= Two_Road_Pos.z + ROAD_SIZE)))
	{
		cl.zombie_list.clear();
		change = MapType::SECOND_PATH;
		cl.map_type = MapType::SECOND_PATH;
		Send_zombie_number_packet(cl._id, ROAD_ZOMBIE_NUM);
		if (MapCheck(MapType::FIRST_PATH))
		{
			remain_zombie_num = ROAD_ZOMBIE_NUM;
		}
	}
	else if ((cl.map_type != MapType::FINAL_PATH) && ((Three_Road_Pos.x <= cl.player->x && cl.player->x <= THREE_Base_Pos.x) && (Three_Road_Pos.z <= cl.player->z && cl.player->z <= Three_Road_Pos.z + ROAD_SIZE)))
	{
		cl.zombie_list.clear();
		change = MapType::FINAL_PATH;
		cl.map_type = MapType::FINAL_PATH;
		Send_zombie_number_packet(cl._id, ROAD_ZOMBIE_NUM);
		if (MapCheck(MapType::FIRST_PATH))
		{
			remain_zombie_num = ROAD_ZOMBIE_NUM;
		}
	}
	else if ((cl.map_type != MapType::EXIT) && ((Exit_Pos.x <= cl.player->x && cl.player->x <= Exit_End_Pos.x) && (Exit_Pos.z <= cl.player->z && cl.player->z <= Exit_End_Pos.z)))
	{
		cl.zombie_list.clear();
		change = MapType::EXIT;
		cl.map_type = MapType::EXIT;
	}

	//cl.zombie_list.clear();
	if (zombie_send == false)
	{
		for (auto& s_cl : g_clients)
		{
			if (s_cl._state != ClientState::INGAME)
			{
				continue;
			}

			AddTimer(s_cl._id, EVENT_TYPE::EVENT_NPC_SEND, 33);
		}

		zombie_send = true;
	}
		
	if (MapCheck(change)) 
	{
		//cout << "���� ������ŵ�ϴ� \n";
		switch (change)
		{
			case MapType::SECOND_PATH:
			{
				for (auto& zom : b_zombie1)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieAllKill(zom);
					}
				}

				for (auto& s_cl : g_clients)
				{
					if (s_cl._state != ClientState::INGAME) continue;

					Send_zombie_all_kill_packet(s_cl._id, MapType::CHECK_POINT_ONE);
				}

				break;
			}
			case MapType::FINAL_PATH:
			{
				for (auto& zom : b_zombie2)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieAllKill(zom);
					}
				}

				for (auto& s_cl : g_clients)
				{
					if (s_cl._state != ClientState::INGAME) continue;

					Send_zombie_all_kill_packet(s_cl._id, MapType::CHECK_POINT_TWO);
				}

				break;
			}
			case MapType::CHECK_POINT_ONE:
			{
				for (auto& zom : r_zombie1)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieAllKill(zom);
					}
				}

				for (auto& s_cl : g_clients)
				{
					if (s_cl._state != ClientState::INGAME) continue;

					Send_zombie_all_kill_packet(s_cl._id, MapType::FIRST_PATH);
				}

				break;
			}
			case MapType::CHECK_POINT_TWO:
			{
				for (auto& zom : r_zombie2)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieAllKill(zom);
					}
				}

				for (auto& s_cl : g_clients)
				{
					if (s_cl._state != ClientState::INGAME) continue;

					Send_zombie_all_kill_packet(s_cl._id, MapType::SECOND_PATH);
				}

				break;
			}
			case MapType::CHECK_POINT_FINAL:
			{
				for (auto& zom : r_zombie3)
				{
					if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
					{
						ZombieAllKill(zom);
					}
				}

				for (auto& s_cl : g_clients)
				{
					if (s_cl._state != ClientState::INGAME) continue;

					Send_zombie_all_kill_packet(s_cl._id, MapType::FINAL_PATH);
				}

				break;
			}
		}

		map_type = cl.map_type;

		if (change == MapType::CHECK_POINT_ONE || change == MapType::CHECK_POINT_TWO || change == MapType::CHECK_POINT_FINAL)
		{
			AddTimer(0, EVENT_TYPE::EVENT_NPC_SPAWN, 4000);
		}
	}
}

void Server::Send_player_attack_packet(int c_id, int a_id, float mx, float mz)
{
	sc_player_attack_packet packet;
	packet.size = sizeof(packet);
	packet.id = a_id;
	packet.type = MsgType::SC_PLAYER_ATTACK;
	packet.mx = mx;
	packet.mz = mz;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

bool Server::ZombieSendInsert(int c_id, void* packet, int size)
{
	if (g_clients[c_id]._zombie_prev_size + size > 1800)
	{
		g_clients[c_id].send_lock.lock();
		if (g_clients[c_id]._zombie_send_overflow)
		{
			g_clients[c_id].send_lock.unlock();
			return false;
		}

		g_clients[c_id]._zombie_send_overflow = true;
		g_clients[c_id].send_lock.unlock();

		for (auto& cl : g_clients)
		{
			if (cl._state != ClientState::INGAME)	continue;

			cl.size_lock.lock();
			cl.do_send(cl._zombie_prev_size, cl.zombie_send_buf);
			cl._zombie_prev_size = 0;
			ZeroMemory(cl.zombie_send_buf, sizeof(cl.zombie_send_buf));
			cl.size_lock.unlock();
		}

		g_clients[c_id]._zombie_send_overflow = false;
		return false;
	}

	g_clients[c_id].size_lock.lock();
	memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, packet, size);
	g_clients[c_id]._zombie_prev_size += size;
	g_clients[c_id].size_lock.unlock();

	return true;
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

	//g_clients[c_id].size_lock.lock();
	//memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	//g_clients[c_id]._zombie_prev_size += packet.size;
	//g_clients[c_id].size_lock.unlock();
	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if(g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
}

void Server::Send_player_reload_packet(int c_id, int s_id)
{
	sc_player_reload_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_RELOAD;
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
	if (npc._state != ZombieState::DEAD)				// ���� �׾����� �ʴٸ� ���� �ش� ���̳� ������ �ذ����� ���� ��
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

			Send_fail_packet(client._id, MsgType::SC_ZOMBIE_REMAIN);	// �׷��� ���� �����ִٰ� ����
		}

		return true;	// �Ѹ����� ���������� �ϴ� �����Ŵ� �ٷ� true��ȯ
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

void Server::Send_player_zombie_kill_num_packet(int c_id, int s_id, int z_num)
{
	sc_player_zombie_klil_packet packet;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_PLAYER_KILL_NUMBER;
	packet.zom_num = z_num;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::PlayerAttack(Client& cl, NPC& npc, MapType m_type, float p_x, float p_z)
{
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
			AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_DEAD, 1000);
			
			//ZombieDead(npc, m_type);
			cl.player->kill_zombie += 1;
			for (auto& other : g_clients)
			{
				if (other._state != ClientState::INGAME) continue;

				Send_player_zombie_kill_num_packet(other._id, cl._id, cl.player->kill_zombie);
				Send_zombie_dead_packet(cl._id, npc._id, m_type);
			}
		}

		//zom.zombie->z_attack_lock.unlock();
		
		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_zombie_info_packet(a_cl._id, npc._id, hp, m_type);
		}
	}

	if(npc._state == ZombieState::SPAWN)
		npc.zombie->hp = hp;
}

void Server::Send_commander_skill_packet(int c_id, int s_id)
{
	sc_commander_special_packet packet;
	packet.bullet = 30;
	packet.hp = g_clients[s_id].player->hp;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_COMMANDER_SPECIAL;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::ResurrectionPlayer(Client& cl)
{
	cl._state = ClientState::INGAME;
	int max_hp = cl.player->maxhp;
	cl.player->hp = max_hp;

	switch (cl.map_type)
	{
	case MapType::FIRST_PATH:
	{
		for (auto& zom : r_zombie1)
		{
			if (zom._state != ZombieState::SPAWN) continue;
			if (NCDis_check(cl._id, zom) == false) continue;

			cl.zombie_list.insert(zom._id);
			Send_viewlist_put_packet(cl._id, zom._id, cl.map_type, r_zombie1[zom._id].zombie->GetX(), r_zombie1[zom._id].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, r_zombie1[zom._id].zombie->_type);
		}

		break;
	}
	case MapType::SECOND_PATH:
	{
		for (auto& zom : r_zombie2)
		{
			if (zom._state != ZombieState::SPAWN) continue;
			if (NCDis_check(cl._id, zom) == false) continue;

			cl.zombie_list.insert(zom._id);
			Send_viewlist_put_packet(cl._id, zom._id, cl.map_type, r_zombie2[zom._id].zombie->GetX(), r_zombie2[zom._id].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, r_zombie2[zom._id].zombie->_type);
		}

		break;
	}
	case MapType::FINAL_PATH:
	{
		for (auto& zom : r_zombie3)
		{
			if (zom._state != ZombieState::SPAWN) continue;
			if (NCDis_check(cl._id, zom) == false) continue;

			cl.zombie_list.insert(zom._id);
			Send_viewlist_put_packet(cl._id, zom._id, cl.map_type, r_zombie3[zom._id].zombie->GetX(), r_zombie3[zom._id].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, r_zombie3[zom._id].zombie->_type);
		}

		break;
	}
	case MapType::CHECK_POINT_ONE:
	{
		for (auto& zom : b_zombie1)
		{
			if (zom._state != ZombieState::SPAWN) continue;
			if (NCDis_check(cl._id, zom) == false) continue;

			cl.zombie_list.insert(zom._id);
			Send_viewlist_put_packet(cl._id, zom._id, cl.map_type, b_zombie1[zom._id].zombie->GetX(), b_zombie1[zom._id].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, b_zombie1[zom._id].zombie->_type);
		}

		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		for (auto& zom : b_zombie2)
		{
			if (zom._state != ZombieState::SPAWN) continue;
			if (NCDis_check(cl._id, zom) == false) continue;

			cl.zombie_list.insert(zom._id);
			Send_viewlist_put_packet(cl._id, zom._id, cl.map_type, b_zombie2[zom._id].zombie->GetX(), b_zombie2[zom._id].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, b_zombie2[zom._id].zombie->_type);
		}

		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		for (auto& zom : b_zombie3)
		{
			if (zom._state != ZombieState::SPAWN) continue;
			if (NCDis_check(cl._id, zom) == false) continue;

			cl.zombie_list.insert(zom._id);
			Send_viewlist_put_packet(cl._id, zom._id, cl.map_type, b_zombie3[zom._id].zombie->GetX(), b_zombie3[zom._id].zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, b_zombie3[zom._id].zombie->_type);
		}

		break;
	}
	}

	for (auto& s_cl : g_clients)
	{
		if (s_cl._state != ClientState::INGAME) continue;

		Send_commander_skill_packet(s_cl._id, cl._id);
	}
}

void Server::Send_commander_skill_check_packet(int c_id, int s_id)
{
	sc_player_co_special_check_packet packet;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_COMMANDER_SPECIAL_CHECK;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::CommanderSpecialSkill(Client& cl)
{
	for (auto& other : g_clients)
	{
		if (other._id == cl._id)
		{
			continue;
		}

		if (other._state != ClientState::DEAD)
		{
			continue;
		}

		if (Distance(cl.player->x, cl.player->z, other.player->x, other.player->z) <= 3.0f)
		{
			ResurrectionPlayer(other);
		}

	}
}

bool Server::EngineerSpecialSkillZombieCheck(int x, int z, Direction dir, NPC& npc)
{
	int root_x = (int)npc.zombie->GetX();
	int root_z = (int)npc.zombie->GetZ();

	float result_root_x = npc.zombie->GetX() - root_x;
	float result_root_z = npc.zombie->GetZ() - root_z;

	if (result_root_x > 0.5f)
		root_x += 1;
	if (result_root_z > 0.5f)
		root_z += 1;

	if (dir == Direction::UP)
	{
		for (int t_z = z + 1; t_z <= z + 5; ++t_z)
		{
			for (int t_x = x - 3; t_x <= x + 3; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}
	}
	else if (dir == Direction::DOWN)
	{
		for (int t_z = z - 5; t_z <= z  - 1; ++t_z)
		{
			for (int t_x = x - 3; t_x <= x + 3; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}
	}
	else if (dir == Direction::RIGHT)
	{
		for (int t_z = z - 3; t_z <= z  + 3; ++t_z)
		{
			for (int t_x = x + 1; t_x <= x + 5; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}
	}
	else if (dir == Direction::LEFT)
	{
		for (int t_z = z - 3; t_z <= z + 3; ++t_z)
		{
			for (int t_x = x - 5; t_x <= x - 1; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}
	}
	else if (dir == Direction::UP_RIGHT)
	{
		x += 2;
		z += 2;

		for (int t_z = z - 2; t_z <= z; ++t_z)
		{
			for (int t_x = x - 3; t_x <= x + 1; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		for (int t_z = z; t_z <= z + 2; ++t_z)
		{
			for (int t_x = x - 1; t_x <= x + 3; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		if (root_x == x - 1 && root_z == z + 4) return false;

		if (root_x == x - 2 && root_z == z + 3) return false;
		if (root_x == x - 1 && root_z == z + 3) return false;
		if (root_x == x && root_z == z + 3) return false;

		if (root_x == x - 4 && root_z == z + 1) return false;
		if (root_x == x + 2 && root_z == z + 1) return false;

		if (root_x == x - 2 && root_z == z - 1) return false;
		if (root_x == x + 4 && root_z == z - 1) return false;

		if (root_x == x && root_z == z - 3) return false;
		if (root_x == x + 1 && root_z == z - 3) return false;
		if (root_x == x + 2 && root_z == z - 3) return false;

		if (root_x == x + 1 && root_z == z - 4) return false;
	}
	else if (dir == Direction::DOWN_LEFT)
	{
		x -= 2;
		z -= 2;

		for (int t_z = z - 2; t_z <= z; ++t_z)
		{
			for (int t_x = x - 3; t_x <= x + 1; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		for (int t_z = z; t_z <= z + 2; ++t_z)
		{
			for (int t_x = x - 1; t_x <= x + 3; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		if (root_x == x - 1 && root_z == z + 4) return false;

		if (root_x == x - 2 && root_z == z + 3) return false;
		if (root_x == x - 1 && root_z == z + 3) return false;
		if (root_x == x && root_z == z + 3) return false;

		if (root_x == x - 4 && root_z == z + 1) return false;
		if (root_x == x + 2 && root_z == z + 1) return false;

		if (root_x == x - 2 && root_z == z - 1) return false;
		if (root_x == x + 4 && root_z == z - 1) return false;

		if (root_x == x && root_z == z - 3) return false;
		if (root_x == x + 1 && root_z == z - 3) return false;
		if (root_x == x + 2 && root_z == z - 3) return false;

		if (root_x == x + 1 && root_z == z - 4) return false;
	}
	else if (dir == Direction::UP_LEFT)
	{
		x -= 2;
		z += 2;

		for (int t_z = z - 2; t_z <= z; ++t_z)
		{
			for (int t_x = x - 1; t_x <= x + 3; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		for (int t_z = z; t_z <= z + 2; ++t_z)
		{
			for (int t_x = x - 3; t_x <= x + 1; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		if (root_x == x + 1 && root_z == z + 4) return false;

		if (root_x == x && root_z == z + 3) return false;
		if (root_x == x + 1 && root_z == z + 3) return false;
		if (root_x == x + 2 && root_z == z + 3) return false;

		if (root_x == x - 2 && root_z == z + 1) return false;
		if (root_x == x + 4 && root_z == z + 1) return false;

		if (root_x == x + 2 && root_z == z - 1) return false;
		if (root_x == x - 4 && root_z == z - 1) return false;

		if (root_x == x && root_z == z - 3) return false;
		if (root_x == x - 1 && root_z == z - 3) return false;
		if (root_x == x - 2 && root_z == z - 3) return false;

		if (root_x == x - 1 && root_z == z - 4) return false;
	}
	else if (dir == Direction::DOWN_RIGHT)
	{
		x += 2;
		z -= 2;

		for (int t_z = z - 2; t_z <= z; ++t_z)
		{
			for (int t_x = x - 1; t_x <= x + 3; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		for (int t_z = z; t_z <= z + 2; ++t_z)
		{
			for (int t_x = x - 3; t_x <= x + 1; ++t_x)
			{
				if (root_x == t_x && root_z == t_z)
					return false;
			}
		}

		if (root_x == x + 1 && root_z == z + 4) return false;

		if (root_x == x && root_z == z + 3) return false;
		if (root_x == x + 1 && root_z == z + 3) return false;
		if (root_x == x + 2 && root_z == z + 3) return false;

		if (root_x == x - 2 && root_z == z + 1) return false;
		if (root_x == x + 4 && root_z == z + 1) return false;

		if (root_x == x + 2 && root_z == z - 1) return false;
		if (root_x == x - 4 && root_z == z - 1) return false;

		if (root_x == x && root_z == z - 3) return false;
		if (root_x == x - 1 && root_z == z - 3) return false;
		if (root_x == x - 2 && root_z == z - 3) return false;

		if (root_x == x - 1 && root_z == z - 4) return false;
	}

	return true;
}

bool Server::EngineerSpecialSkillMapCheck(int x, int z, Direction dir)
{
	if (dir == Direction::UP || dir == Direction::DOWN)
	{
		for (int j = 0; j < 3; ++j)
		{
			for (int i = 0; i < 5; ++i)
			{
				if (map.map[z -1 +j][x - 2 + i] != (char)MazeWall::ROAD)
				{
					return false;
				}
			}
		}
	}
	else if(dir == Direction::RIGHT || dir == Direction::LEFT)
	{
		for (int j = 0; j < 3; ++j)
		{
			for (int i = 0; i < 5; ++i)
			{
				if (map.map[z - 2 + i][x - 1 + j] != (char)MazeWall::ROAD)
				{
					return false;
				}
			}
		}
	}
	else if (dir == Direction::UP_LEFT || dir == Direction::DOWN_RIGHT)
	{
		for (int t_z = z - 2; t_z <= z; ++t_z)
		{
			for (int t_x = x - 2; t_x <= x; ++t_x)
			{
				if (map.map[t_z][t_x] == (char)MazeWall::BARRICADE)
					return false;
				if (map.map[t_z][t_x] == (char)MazeWall::WALL)
					return false;
			}
		}

		for (int t_z = z; t_z <= z + 2; ++t_z)
		{
			for (int t_x = x; t_x <= x + 2; ++t_x)
			{
				if (map.map[t_z][t_x] == (char)MazeWall::BARRICADE)
					return false;
				if (map.map[t_z][t_x] == (char)MazeWall::WALL)
					return false;
			}
		}

		if (map.map[z - 3][x - 1] == (char)MazeWall::BARRICADE || map.map[z - 3][x - 1] == (char)MazeWall::WALL)
			return false;

		if (map.map[z - 1][x - 3] == (char)MazeWall::BARRICADE || map.map[z - 1][x - 3] == (char)MazeWall::WALL)
			return false;
		if (map.map[z - 1][x + 1] == (char)MazeWall::BARRICADE || map.map[z - 1][x + 1] == (char)MazeWall::WALL)
			return false;

		if (map.map[z + 1][x - 1] == (char)MazeWall::BARRICADE || map.map[z + 1][x - 1] == (char)MazeWall::WALL)
			return false;
		if (map.map[z + 1][x + 3] == (char)MazeWall::BARRICADE || map.map[z + 1][x + 3] == (char)MazeWall::WALL)
			return false;

		if (map.map[z + 3][x + 1] == (char)MazeWall::BARRICADE || map.map[z + 3][x + 1] == (char)MazeWall::WALL)
			return false;
	}
	else if(dir == Direction::UP_RIGHT || dir == Direction::DOWN_LEFT)
	{
		for (int t_z = z; t_z <= z + 2; ++t_z)
		{
			for (int t_x = x - 2; t_x <= x; ++t_x)
			{
				if (map.map[t_z][t_x] == (char)MazeWall::BARRICADE)
					return false;
				if (map.map[t_z][t_x] == (char)MazeWall::WALL)
					return false;
			}
		}

		for (int t_z = z - 2; t_z <= z; ++t_z)
		{
			for (int t_x = x; t_x <= x + 2; ++t_x)
			{
				if (map.map[t_z][t_x] == (char)MazeWall::BARRICADE)
					return false;
				if (map.map[t_z][t_x] == (char)MazeWall::WALL)
					return false;
			}
		}

		if (map.map[z + 3][x + 1] == (char)MazeWall::BARRICADE || map.map[z + 3][x + 1] == (char)MazeWall::WALL)
			return false;

		if (map.map[z - 1][x - 1] == (char)MazeWall::BARRICADE || map.map[z - 1][x - 1] == (char)MazeWall::WALL)
			return false;
		if (map.map[z - 1][x + 3] == (char)MazeWall::BARRICADE || map.map[z - 1][x + 3] == (char)MazeWall::WALL)
			return false;

		if (map.map[z + 1][x + 1] == (char)MazeWall::BARRICADE || map.map[z + 1][x + 1] == (char)MazeWall::WALL)
			return false;
		if (map.map[z + 1][x - 3] == (char)MazeWall::BARRICADE || map.map[z + 1][x - 3] == (char)MazeWall::WALL)
			return false;

		if (map.map[z + 3][x - 1] == (char)MazeWall::BARRICADE || map.map[z + 3][x - 1] == (char)MazeWall::WALL)
			return false;
	}

	return true;
}

void Server::EngineerBuildBarricade(int bx, int bz, Direction dir)
{
	if (dir == Direction::RIGHT || dir == Direction::LEFT)
	{
		for (int j = 0; j < 3; ++j)
		{
			for (int i = 0; i < 5; ++i)
			{
				map.map[bz - 2 + i][bx - 1 + j] = (char)MazeWall::BARRICADE;
			}
		}

		as_map.EditMap(bx - 1, bz - 2, bx + 1, bz + 2, map);
	}
	else if (dir == Direction::UP || dir == Direction::DOWN)
	{
		for (int j = 0; j < 3; ++j)
		{
			for (int i = 0; i < 5; ++i)
			{
				map.map[bz - 1 + j][bx - 2 + i] = (char)MazeWall::BARRICADE;
			}
		}

		as_map.EditMap(bx - 2, bz - 2, bx + 2, bz + 2, map);
	}
	else if (dir == Direction::UP_LEFT || dir == Direction::DOWN_RIGHT)
	{
		for (int t_z = bz - 2; t_z <= bz; ++t_z)
		{
			for (int t_x = bx - 2; t_x <= bx; ++t_x)
			{
				map.map[t_z][t_x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int t_z = bz; t_z <= bz + 2; ++t_z)
		{
			for (int t_x = bx; t_x <= bx + 2; ++t_x)
			{
				map.map[t_z][t_x] = (char)MazeWall::BARRICADE;
			}
		}

		map.map[bz - 3][bx - 1] = (char)MazeWall::BARRICADE;

		map.map[bz - 1][bx - 3] = (char)MazeWall::BARRICADE;
		map.map[bz - 1][bx + 1] = (char)MazeWall::BARRICADE;

		map.map[bz + 1][bx - 1] = (char)MazeWall::BARRICADE;
		map.map[bz + 1][bx + 3] = (char)MazeWall::BARRICADE;

		map.map[bz + 3][bx + 1] = (char)MazeWall::BARRICADE;

		as_map.EditMap(bx - 3, bz - 3, bx + 3, bz + 3, map);
	}
	else if (dir == Direction::UP_RIGHT || dir == Direction::DOWN_LEFT)
	{
		for (int t_z = bz; t_z <= bz + 2; ++t_z)
		{
			for (int t_x = bx - 2; t_x <= bx; ++t_x)
			{
				map.map[t_z][t_x] = (char)MazeWall::BARRICADE;
			}
		}

		for (int t_z = bz - 2; t_z <= bz; ++t_z)
		{
			for (int t_x = bx; t_x <= bx + 2; ++t_x)
			{
				map.map[t_z][t_x] = (char)MazeWall::BARRICADE;
			}
		}

		map.map[bz - 3][bx + 1] = (char)MazeWall::BARRICADE;

		map.map[bz - 1][bx - 1] = (char)MazeWall::BARRICADE;
		map.map[bz - 1][bx + 3] = (char)MazeWall::BARRICADE;

		map.map[bz + 1][bx + 1] = (char)MazeWall::BARRICADE;
		map.map[bz + 1][bx - 3] = (char)MazeWall::BARRICADE;

		map.map[bz + 3][bx - 1] = (char)MazeWall::BARRICADE;

		as_map.EditMap(bx - 3, bz - 3, bx + 3, bz + 3, map);
	}
}

void Server::Send_engineer_skill_packet(int c_id, int s_id, int t_x, int t_z)
{
	sc_engineer_barrigate_build_packet packet;
	packet.dir = g_clients[s_id].player->dir;
	packet.x = t_x;
	packet.z = t_z;
	packet.id = s_id;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ENGINEER_SPECIAL;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_engineer_skill_check_packet(int c_id, int x, int z)
{
	sc_player_en_special_check_packet packet;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ENGINEER_SPECIAL_CHECK;
	packet.x = x;
	packet.z = z;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::EngineerSpecialSkill(Client& cl)
{
	bool check = true;	// true : ������ ����, false : ���� �� ����
	int t_x = 0, t_z = 0;

	int cl_root_x = (int)cl.player->x;
	int cl_root_z = (int)cl.player->z;

	float cl_result_root_x = cl.player->x - cl_root_x;
	float cl_result_root_z = cl.player->z - cl_root_z;

	if (cl_result_root_x > 0.5f)
		cl_root_x += 1;
	if (cl_result_root_z > 0.5f)
		cl_root_z += 1;

	if (map_type != MapType::CHECK_POINT_ONE && map_type != MapType::CHECK_POINT_TWO && map_type != MapType::CHECK_POINT_FINAL)
	{
		Send_fail_packet(cl._id, MsgType::SC_ENGINEER_SPECIAL_BUILD_FAIL);
		return;
	}

	// ��ġ ������ �Ǵ���??
	{
		if (cl.player->dir == Direction::UP)
		{
			t_x = cl_root_x;
			t_z = cl_root_z + 3;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::UP);
		}
		else if (cl.player->dir == Direction::UP_RIGHT)
		{
			t_x = cl_root_x + 3;
			t_z = cl_root_z + 3;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::UP_RIGHT);
		}
		else if (cl.player->dir == Direction::RIGHT)
		{
			t_x = cl_root_x + 3;
			t_z = cl_root_z;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::RIGHT);
		}
		else if (cl.player->dir == Direction::DOWN_RIGHT)
		{
			t_x = cl_root_x + 3;
			t_z = cl_root_z - 3;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::DOWN_RIGHT);
		}
		else if (cl.player->dir == Direction::DOWN)
		{
			t_x = cl_root_x;
			t_z = cl_root_z - 3;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::DOWN);
		}
		else if (cl.player->dir == Direction::DOWN_LEFT)
		{
			t_x = cl_root_x - 3;
			t_z = cl_root_z - 3;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::DOWN_LEFT);
		}
		else if (cl.player->dir == Direction::LEFT)
		{
			t_x = cl_root_x - 3;
			t_z = cl_root_z;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::LEFT);
		}
		else if (cl.player->dir == Direction::UP_LEFT)
		{
			t_x = cl_root_x - 3;
			t_z = cl_root_z + 3;
			check = EngineerSpecialSkillMapCheck(t_x, t_z, Direction::UP_LEFT);
		}
	}

	if (check == false)
	{
		Send_fail_packet(cl._id, MsgType::SC_ENGINEER_SPECIAL_BUILD_FAIL);
		return;
	}

	if (map_type == MapType::CHECK_POINT_ONE)
	{
		for (auto& npc : b_zombie1)
		{
			if (npc._state != ZombieState::SPAWN) continue;

			check = EngineerSpecialSkillZombieCheck(cl_root_x, cl_root_z, cl.player->dir, npc);
		}
	}
	else if (map_type == MapType::CHECK_POINT_TWO)
	{
		for (auto& npc : b_zombie2)
		{
			if (npc._state != ZombieState::SPAWN) continue;

			check = EngineerSpecialSkillZombieCheck(cl_root_x, cl_root_z, cl.player->dir, npc);
		}
	}
	else if (map_type == MapType::CHECK_POINT_FINAL)
	{
		for (auto& npc : b_zombie3)
		{
			if (npc._state != ZombieState::SPAWN) continue;

			check = EngineerSpecialSkillZombieCheck(cl_root_x, cl_root_z, cl.player->dir, npc);
		}
	}

	if (check)
	{
		EngineerBuildBarricade(t_x, t_z, cl.player->dir);

		if (map_type == MapType::CHECK_POINT_ONE)
		{
			for (auto& npc : b_zombie1)
			{
				if (npc._state != ZombieState::SPAWN) continue;

				npc.zombie->astar.Map(as_map);
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}
		else if (map_type == MapType::CHECK_POINT_TWO)
		{
			for (auto& npc : b_zombie2)
			{
				if (npc._state != ZombieState::SPAWN) continue;

				npc.zombie->astar.Map(as_map);
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}
		else if (map_type == MapType::CHECK_POINT_FINAL)
		{
			for (auto& npc : b_zombie3)
			{
				if (npc._state != ZombieState::SPAWN) continue;

				npc.zombie->astar.Map(as_map);
				npc.astar_check = false;
				npc.zombie->astar.Delete();
			}
		}

		for (auto& other : g_clients)
		{
			if (other._state != ClientState::INGAME) continue;

			Send_engineer_skill_packet(other._id, cl._id, t_x, t_z);
		}
		
	}
	else
	{
		Send_fail_packet(cl._id, MsgType::SC_ENGINEER_SPECIAL_BUILD_FAIL);
	}

}

void Server::MercenarySpecialSkill(Client& cl)
{

}

void Server::Send_gm_change_map_packet(int c_id, int s_id, int x, int z)
{
	sc_gm_change_map_packet packet;
	packet.size = sizeof(packet);
	packet.id = s_id;
	packet.type = MsgType::SC_GM_MAP_CHANGE_MAP;
	packet.x = x;
	packet.z = z;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::Send_gm_hp_packet(int c_id, int s_id)
{
	sc_gm_player_hp_packet packet;
	packet.size = sizeof(packet);
	packet.id = s_id;
	packet.hp = g_clients[s_id].player->hp;
	packet.type = MsgType::SC_GM_PLAYER_HP_UP;
	g_clients[c_id].do_send(sizeof(packet), &packet);
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
		Send_login_ok_packet(client_id);

		// Ŭ���̾�Ʈ�� �����Ͽ����Ƿ� INGAME ���·� ��ȯ
		Client& cl = g_clients[client_id];
		cl._state = ClientState::INGAME;

		// Ŭ���̾�Ʈ�� ������ �������� ���������� �����Ƿ� ������������ ����
		cl.map_type = MapType::SPAWN;

		// �ٸ� �÷��̾�� �α��� ���̵�
		
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			if (ClientState::INGAME != other._state)
			{
				continue;
			}

			sc_login_other_packet login_packet;
			login_packet.id = client_id;
			login_packet.size = sizeof(login_packet);
			login_packet.type = MsgType::SC_LOGIN_OTHER;
			other.do_send(sizeof(login_packet), &login_packet);
		}

		// �ڱ� �ڽſ��� �ٸ� �÷��̾���� ���̵�, ���� �޾ƿ���
		for (auto& other : g_clients)
		{
			if (other._id == client_id)
				continue;

			if (ClientState::INGAME != other._state)
			{
				continue;
			}

			sc_login_other_packet login_packet;
			login_packet.id = other._id;
			login_packet.size = sizeof(login_packet);
			login_packet.type = MsgType::SC_LOGIN_OTHER;
			cl.do_send(sizeof(login_packet), &login_packet);
		}

		break;
	}
	case (int)MsgType::CS_PLAYER_ATTACK:
	{
		cs_attack_packet* packet = reinterpret_cast<cs_attack_packet*>(p);

		if (cl._state != ClientState::INGAME) break;

		// �Ѿ��� �Ҹ�ǰ� ������ �Ͽ��� ������ ��ο��� ������ �Ǿ��ٰ� ����
		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_player_attack_packet(a_cl._id, cl._id, packet->mx, packet->mz);
		}

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
		// ���⼭ �ֺ� �繰�� �˻��ؾ���
		// ������ DOOR �ۿ� �������� �ʾ� �ش� �ڵ常 �ۼ�

		// ���� DOOR�� �����Ͼ �۵� �����Ͽ� �����Ͼ �ƴ϶�� �Է��ص� �ƹ��� ȿ���� ���� ��������
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
			//	if (ZombieRemain(zom))		// �׷��� ���࿡ ���������� true�̹Ƿ� �Լ��� �׳� �������´�.
			//		return;
			//}

			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				if (ZombieRemain(zom))		// �׷��� ���࿡ ���������� true�̹Ƿ� �Լ��� �׳� �������´�.
					return;
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				if (ZombieRemain(zom))		// �׷��� ���࿡ ���������� true�̹Ƿ� �Լ��� �׳� �������´�.
					return;
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			//for (auto& zom : b_zombie1)
			//{
			//	if (ZombieRemain(zom))		// �׷��� ���࿡ ���������� true�̹Ƿ� �Լ��� �׳� �������´�.
			//		return;
			//}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				if (ZombieRemain(zom))		// �׷��� ���࿡ ���������� true�̹Ƿ� �Լ��� �׳� �������´�.
					return;
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				if (ZombieRemain(zom))		// �׷��� ���࿡ ���������� true�̹Ƿ� �Լ��� �׳� �������´�.
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
		for (auto& s_cl : g_clients)
		{
			//s_cl.state_lock.lock();
			if (s_cl._state != ClientState::INGAME)
			{
			//	s_cl.state_lock.unlock();
				continue;
			}
			//s_cl.state_lock.unlock();

			Send_player_reload_packet(s_cl._id, cl._id);
		}
		break;
	}
	case (int)MsgType::CS_PLAYER_ROTATE:
	{
		if (cl._state != ClientState::INGAME)
			break;

		cs_rotate_packet* packet = reinterpret_cast<cs_rotate_packet*>(p);

		// �÷��̾� ���� ���ϱ�
		{
			if (0.75f < packet->mx && packet->mx <= 1.0f)
			{
				cl.player->dir = Direction::RIGHT;
			}
			else if (0.25f < packet->mx && packet->mx <= 0.75f)
			{
				if (packet->mz > 0.0f)
				{
					cl.player->dir = Direction::DOWN_RIGHT;
				}
				else
				{
					cl.player->dir = Direction::UP_RIGHT;
				}
			}
			else if (-0.25f < packet->mx && packet->mx <= 0.25f)
			{
				if (packet->mz > 0.0f)
				{
					cl.player->dir = Direction::DOWN;
				}
				else
				{
					cl.player->dir = Direction::UP;
				}
			}
			else if (-0.75f < packet->mx && packet->mx <= -0.25f)
			{
				if (packet->mz > 0.0f)
				{
					cl.player->dir = Direction::DOWN_LEFT;
				}
				else
				{
					cl.player->dir = Direction::UP_LEFT;
				}
			}
			else
			{
				cl.player->dir = Direction::LEFT;
			}
		}

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
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		cs_move_packet* packet = reinterpret_cast<cs_move_packet*>(p);

		for (auto &other : g_clients)
		{
			if (other._state != ClientState::INGAME) continue;

			Send_player_move_packet(other._id, client_id, packet->x, packet->z, packet->t_x, packet->t_z, packet->speed, packet->rotation_angle, packet->in_input);
		}

		cl.move_lock.lock();
		int pre_x = (int)cl.player->x;
		int pre_z = (int)cl.player->z;
		cl.player->x = packet->x;
		cl.player->z = packet->z;
		cl.move_lock.unlock();
		
		// �̸� �ֺ� �丮��Ʈ Ȯ��
		unordered_set<int> near_zombie_list;
		switch (cl.map_type)
		{
		case MapType::FIRST_PATH:
		{
			for (auto& zom : r_zombie1)
			{
				if (zom._state != ZombieState::SPAWN)	// ���� �������� �����Ƿ� �丮��Ʈ�� �������� ����.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// ���� �����־ �丮��Ʈ ���� ���̸� �׸����� ����.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				if (zom._state != ZombieState::SPAWN)	// ���� �������� �����Ƿ� �丮��Ʈ�� �������� ����.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// ���� �����־ �丮��Ʈ ���� ���̸� �׸����� ����.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				if (zom._state != ZombieState::SPAWN)	// ���� �������� �����Ƿ� �丮��Ʈ�� �������� ����.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// ���� �����־ �丮��Ʈ ���� ���̸� �׸����� ����.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			for (auto& zom : b_zombie1)
			{
				if (zom._state != ZombieState::SPAWN)	// ���� �������� �����Ƿ� �丮��Ʈ�� �������� ����.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// ���� �����־ �丮��Ʈ ���� ���̸� �׸����� ����.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				if (zom._state != ZombieState::SPAWN)	// ���� �������� �����Ƿ� �丮��Ʈ�� �������� ����.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// ���� �����־ �丮��Ʈ ���� ���̸� �׸����� ����.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				if (zom._state != ZombieState::SPAWN)	// ���� �������� �����Ƿ� �丮��Ʈ�� �������� ����.
					continue;
				if (NCDis_check(cl._id, zom) == false)	// ���� �����־ �丮��Ʈ ���� ���̸� �׸����� ����.
					continue;

				near_zombie_list.insert(zom._id);
			}

			break;
		}
		}

		//cout << "client_id [ " << client_id << "] x : " << cl.player->x << ", z : " << cl.player->z << "\n";

		cl.list_lock.lock();
		unordered_set<int> cl_z_list = cl.zombie_list;
		cl.list_lock.unlock();

		// �̵� ���� �� �ֺ��� ���� ������� �丮��Ʈ�� �ִ´�.
		for (auto zom : near_zombie_list)	// ����� ���� ����Ʈ
		{
			if (0 == cl_z_list.count(zom))	// �ٵ� ���� Ŭ���� ���� ����Ʈ�� ���� �ʴٸ�?
			{
				cl.list_lock.lock();
				cl.zombie_list.insert(zom);
				cl.list_lock.unlock();

				switch (cl.map_type)
				{
				case MapType::FIRST_PATH:
				{
					cout << "�� �� �����µ� \n";
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

		bool con = false;
		// �̵����� �� �⺻�� �־��µ� ������ �������ٸ�
		for (auto& zom : cl_z_list)	// �� ����Ʈ
		{
			if (0 == near_zombie_list.count(zom))	// �ٵ� ���� ���ٸ�?
			{
				con = false;

				cl.list_lock.lock();
				cl.zombie_list.erase(zom);
				cl.list_lock.unlock();

				switch (map_type)
				{
				case MapType::FIRST_PATH:
				{
					if (r_zombie1[zom]._state == ZombieState::DEAD)
						con = true;
					break;
				}
				case MapType::SECOND_PATH:
				{
					if (r_zombie2[zom]._state == ZombieState::DEAD)
						con = true;
					break;
				}
				case MapType::FINAL_PATH:
				{
					if (r_zombie3[zom]._state == ZombieState::DEAD)
						con = true;
					break;
				}
				case MapType::CHECK_POINT_ONE:
				{
					if (b_zombie1[zom]._state == ZombieState::DEAD)
						con = true;
					break;
				}
				case MapType::CHECK_POINT_TWO:
				{
					if (b_zombie2[zom]._state == ZombieState::DEAD)
						con = true;
					break;
				}
				case MapType::CHECK_POINT_FINAL:
				{
					if (b_zombie3[zom]._state == ZombieState::DEAD)
						con = true;
					break;
				}
				}

				if (con)
				{
					continue;
				}

				Send_viewlist_remove_packet(cl._id, zom, cl.map_type);
			}
		}

		// �̵� �� �ֺ��� ���� ���� ��� �ش� Ŭ�󿡰� �ش� ���� ������

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

			//if (escape == false)
			//{
			//	Send_fail_packet(cl._id, MsgType::SC_PLAYER_SEARCH_FAIL);
			//}

			ChangeMapType(cl);

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
			select_type += 1;
			cl.player = new Commander;
			break;
		}
		case PlayerType::ENGINEER:
		{
			select_type += 2;
			cl.player = new Engineer;
			break;
		}
		case PlayerType::MERCENARY:
		{
			select_type += 4;
			cl.player = new Mercynary;
			break;
		}
		default:
		{
			cout << "�÷��̾� Ÿ���� �������� �ʾҽ��ϴ� \n";
		}
		}

		Send_select_packet(cl._id, cl._id, select_type);

		// �ٸ� �÷��̾�� �ڽ��� ������ ĳ���� ������
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

			Send_select_packet(cl._id, other._id, select_type);
		}

		// �ٸ� �÷��̾ ������ �� �ڽſ��� ������
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

			Send_select_packet(other._id, cl._id, select_type);
		}
		break;
	}
	case (int)MsgType::CS_PLAYER_SPECIAL:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		if (cl.player->special_skill == 0)
		{
			Send_fail_packet(cl._id, MsgType::SC_PLAYER_SPECIAL_NUM_ZERO);
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
	case (int)MsgType::CS_SPECIAL_SKILL_CHANGE:
	{
		cs_special_req_packet* packet = reinterpret_cast<cs_special_req_packet*>(p);

		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		if (cl._type == PlayerType::COMMANDER)
		{
			for (auto& other : g_clients)
			{
				if (other._id == cl._id) continue;
				if (other._state != ClientState::DEAD) continue;
				if (other._id == packet->id) continue;

				if (Distance(cl.player->x, cl.player->z, other.player->x, other.player->z) <= 2.0f)
				{
					Send_commander_skill_check_packet(cl._id, other._id);
				}
			}
		}
		else if (cl._type == PlayerType::ENGINEER)
		{

		}
		else if (cl._type == PlayerType::MERCENARY)
		{

		}


		break;
	}
	case (int)MsgType::CS_SPECIAL_SKILL_REQUEST:
	{
		cs_special_req_packet* packet = reinterpret_cast<cs_special_req_packet*>(p);

		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		if (cl._type == PlayerType::COMMANDER)
		{
			ResurrectionPlayer(g_clients[packet->id]);
		}
		else if (cl._type == PlayerType::ENGINEER)
		{

		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			
		}

		break;
	}
	case (int)MsgType::CS_BARRICADE_REQUEST:
	{
		bool barricade_build = true;
		
		for (auto& client : g_clients)
		{
			if (ClientState::INGAME != client._state)
			{
				Send_fail_packet(cl._id, MsgType::SC_GAME_START_FAIL);
				barricade_build = false;
				break;
			}
		}
		
		if (barricade_build && (cl.send_start_packet == false))
		{
			Send_barricade_packet(cl._id);
		}
		break;
	}
	case (int)MsgType::CS_GAME_START_REQUEST:
	{
		cl.send_start_packet = true;

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
		cout << cl._id << " : ���� ���� ���� \n";

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
	case (int)MsgType::CS_GM_MAP_CHANGE_ROAD_ONE:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		remain_zombie_num = ROAD_ZOMBIE_NUM;

		if (cl._type == PlayerType::COMMANDER)
		{
			cl.player->x = 43.0f;
			cl.player->z = 91.0f;
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			cl.player->x = 44.0f;
			cl.player->z = 91.0f;
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			cl.player->x = 45.0f;
			cl.player->z = 91.0f;
		}

		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_gm_change_map_packet(a_cl._id, cl._id, (int)cl.player->x, (int)cl.player->z);
		}

		break;
	}
	case (int)MsgType::CS_GM_MAP_CHANGE_ROAD_TWO:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		remain_zombie_num = ROAD_ZOMBIE_NUM;

		if (cl._type == PlayerType::COMMANDER)
		{
			cl.player->x = 323.0f;
			cl.player->z = 338.0f;
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			cl.player->x = 323.0f;
			cl.player->z = 339.0f;
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			cl.player->x = 323.0f;
			cl.player->z = 340.0f;
		}

		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_gm_change_map_packet(a_cl._id, cl._id, (int)cl.player->x, (int)cl.player->z);
		}

		break;
	}
	case (int)MsgType::CS_GM_MAP_CHANGE_ROAD_THREE:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		remain_zombie_num = ROAD_ZOMBIE_NUM;

		if (cl._type == PlayerType::COMMANDER)
		{
			cl.player->x = 634.0f;
			cl.player->z = 338.0f;
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			cl.player->x = 634.0f;
			cl.player->z = 339.0f;
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			cl.player->x = 634.0f;
			cl.player->z = 340.0f;
		}

		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_gm_change_map_packet(a_cl._id, cl._id, (int)cl.player->x, (int)cl.player->z);
		}

		break;
	}
	case (int)MsgType::CS_GM_MAP_CHANGE_BASE_ONE:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		remain_zombie_num = FIRST_CHECK_POINT_ZOMBIE_NUM;

		if (cl._type == PlayerType::COMMANDER)
		{
			cl.player->x = 200.0f;
			cl.player->z = 244.0f;
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			cl.player->x = 201.0f;
			cl.player->z = 244.0f;
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			cl.player->x = 202.0f;
			cl.player->z = 244.0f;
		}

		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_gm_change_map_packet(a_cl._id, cl._id, (int)cl.player->x, (int)cl.player->z);
		}

		break;
	}
	case (int)MsgType::CS_GM_MAP_CHANGE_BASE_TWO:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		remain_zombie_num = TWO_CHECK_POINT_ZOMBIE_NUM;

		if (cl._type == PlayerType::COMMANDER)
		{
			cl.player->x = 512.0f;
			cl.player->z = 338.0f;
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			cl.player->x = 512.0f;
			cl.player->z = 339.0f;
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			cl.player->x = 512.0f;
			cl.player->z = 340.0f;
		}

		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_gm_change_map_packet(a_cl._id, cl._id, (int)cl.player->x, (int)cl.player->z);
		}

		break;
	}
	case (int)MsgType::CS_GM_MAP_CHANGE_BASE_THREE:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		remain_zombie_num = THREE_CHECK_POINT_ZOMBIE_NUM;

		if (cl._type == PlayerType::COMMANDER)
		{
			cl.player->x = 823.0f;
			cl.player->z = 338.0f;
		}
		else if (cl._type == PlayerType::ENGINEER)
		{
			cl.player->x = 823.0f;
			cl.player->z = 339.0f;
		}
		else if (cl._type == PlayerType::MERCENARY)
		{
			cl.player->x = 823.0f;
			cl.player->z = 340.0f;
		}

		for (auto& a_cl : g_clients)
		{
			if (a_cl._state != ClientState::INGAME) continue;

			Send_gm_change_map_packet(a_cl._id, cl._id, (int)cl.player->x, (int)cl.player->z);
		}

		break;
	}
	case (int)MsgType::CS_GM_ZOMBIE_ALL_KILL:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		switch (cl.map_type)
		{
		case MapType::FIRST_PATH:
		{
			for (auto& zom : r_zombie1)
			{
				if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
				{
					ZombieAllKill(zom);
				}
			}

			for (auto& s_cl : g_clients)
			{
				if (s_cl._state != ClientState::INGAME) continue;

				Send_zombie_all_kill_packet(s_cl._id, MapType::FIRST_PATH);
			}

			break;
		}
		case MapType::SECOND_PATH:
		{
			for (auto& zom : r_zombie2)
			{
				if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
				{
					ZombieAllKill(zom);
				}
			}

			for (auto& s_cl : g_clients)
			{
				if (s_cl._state != ClientState::INGAME) continue;

				Send_zombie_all_kill_packet(s_cl._id, MapType::SECOND_PATH);
			}

			break;
		}
		case MapType::FINAL_PATH:
		{
			for (auto& zom : r_zombie3)
			{
				if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
				{
					ZombieAllKill(zom);
				}
			}

			for (auto& s_cl : g_clients)
			{
				if (s_cl._state != ClientState::INGAME) continue;

				Send_zombie_all_kill_packet(s_cl._id, MapType::FINAL_PATH);
			}

			break;
		}
		case MapType::CHECK_POINT_ONE:
		{
			for (auto& zom : b_zombie1)
			{
				if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
				{
					ZombieAllKill(zom);
				}
			}

			for (auto& s_cl : g_clients)
			{
				if (s_cl._state != ClientState::INGAME) continue;

				Send_zombie_all_kill_packet(s_cl._id, MapType::CHECK_POINT_ONE);
			}

			break;
		}
		case MapType::CHECK_POINT_TWO:
		{
			for (auto& zom : b_zombie2)
			{
				if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
				{
					ZombieAllKill(zom);
				}
			}

			for (auto& s_cl : g_clients)
			{
				if (s_cl._state != ClientState::INGAME) continue;

				Send_zombie_all_kill_packet(s_cl._id, MapType::CHECK_POINT_TWO);
			}

			break;
		}
		case MapType::CHECK_POINT_FINAL:
		{
			for (auto& zom : b_zombie3)
			{
				if (zom._state == ZombieState::SLEEP || zom._state == ZombieState::SPAWN)
				{
					ZombieAllKill(zom);
				}
			}

			for (auto& s_cl : g_clients)
			{
				if (s_cl._state != ClientState::INGAME) continue;

				Send_zombie_all_kill_packet(s_cl._id, MapType::CHECK_POINT_FINAL);
			}

			break;
		}
		}

		break;
	}
	case (int)MsgType::CS_GM_PLAYER_HP_UP:
	{
		if (cl._state != ClientState::INGAME)
		{
			break;
		}

		int maxhp = cl.player->maxhp;
		cl.player->hp = maxhp;

		for (auto& other : g_clients)
		{
			if (other._state != ClientState::INGAME)
				continue;

			Send_gm_hp_packet(other._id, cl._id);
		}

		break;
	}
	default:
	{
		cout << "�߸����� \n";
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

	// g_clients[c_id].size_lock.lock();
	// memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	// g_clients[c_id]._zombie_prev_size += packet.size;
	// g_clients[c_id].size_lock.unlock();

	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if (g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
}

bool Server::NCDis_check(int c_id,  NPC& npc)
{
	if (CAMERA_WIDTH < abs(g_clients[c_id].player->x - npc.zombie->x)) return false;
	if (CAMERA_HEIGHT < abs(g_clients[c_id].player->z - npc.zombie->z)) return false;

	return true;	// NPC�� CLIENT�� ������ �����Ƿ� zombie_list�� �־�� �Ѵ�.
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
		// �Ÿ� �̳��� ������ ���� Postť �ؼ� ZombieSpawn���� �������ϳ�?
		//cout << "zombie id : " << npc._id << "�� x : " << npc.zombie->x << ", z : " << npc.zombie->z << "�� ������ \n";

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

			// ���� viewlist�� ������ �ʾƼ� �ش� �κ��� ��������
			// viewlist�ȿ� ���� �ִ� ��쿡�� spawn ������ �˷�����
			// ���� ���� ���� ������ ī�޶�ũ�⿡ ���� ���ǰ� ���� �ȵǾ�������, ��� ���� �𸣱⿡ �׳� ��� �����Ѱ� Ŭ���̾�Ʈ���� �ѱ�
			// viewlist �ȿ� ������ Ŭ���̾�Ʈ�� ���� ������ ����
			
			if (NCDis_check(cl._id, npc) == false)	// �Ÿ� �ָ� ���� ������ �����Ƿ�... continue
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

					// ���� viewlist�� ������ �ʾƼ� �ش� �κ��� ��������
					// viewlist�ȿ� ���� �ִ� ��쿡�� spawn ������ �˷�����
					// ���� ���� ���� ������ ī�޶�ũ�⿡ ���� ���ǰ� ���� �ȵǾ�������, ��� ���� �𸣱⿡ �׳� ��� �����Ѱ� Ŭ���̾�Ʈ���� �ѱ�
					// viewlist �ȿ� ������ Ŭ���̾�Ʈ�� ���� ������ ����
					if (NCDis_check(cl._id, b_zombie1[i]) == false)	// �Ÿ� �ָ� ���� ������ �����Ƿ�... continue
						continue;

					cl.list_lock.lock();
					cl.zombie_list.insert(i);
					cl.list_lock.unlock();

					Send_viewlist_put_packet(cl._id, i, map_type, b_zombie1[i].zombie->GetX(), b_zombie1[i].zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, b_zombie1[i].zombie->_type);
				}
				//cout << i << "��° ���� ���� x : " << b_zombie1[i].zombie->GetX() << ", z : " << b_zombie1[i].zombie->GetZ() << "�� �����Ͽ����ϴ� \n";

				AddTimer(i, EVENT_TYPE::EVENT_NPC_MOVE, 667);
			}
			else
			{
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

					// ���� viewlist�� ������ �ʾƼ� �ش� �κ��� ��������
					// viewlist�ȿ� ���� �ִ� ��쿡�� spawn ������ �˷�����
					// ���� ���� ���� ������ ī�޶�ũ�⿡ ���� ���ǰ� ���� �ȵǾ�������, ��� ���� �𸣱⿡ �׳� ��� �����Ѱ� Ŭ���̾�Ʈ���� �ѱ�
					// viewlist �ȿ� ������ Ŭ���̾�Ʈ�� ���� ������ ����

					if (NCDis_check(cl._id, b_zombie2[i]) == false)	// �Ÿ� �ָ� ���� ������ �����Ƿ�... continue
						continue;

					cl.list_lock.lock();
					cl.zombie_list.insert(i);
					cl.list_lock.unlock();

					Send_viewlist_put_packet(cl._id, i, map_type, b_zombie2[i].zombie->GetX(), b_zombie2[i].zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, b_zombie2[i].zombie->_type);
				}
				//cout << i << "��° ���� ���� x : " << b_zombie2[i].zombie->GetX() << ", z : " << b_zombie2[i].zombie->GetZ() << "�� �����Ͽ����ϴ� \n";
				AddTimer(i, EVENT_TYPE::EVENT_NPC_MOVE, 667);
			}
			else
			{
				cout << "�̹� �����Ǿ� �ְų� �׾� �ֽ��ϴ�.(�׷� �� ���µ�?) \n";
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

					// ���� viewlist�� ������ �ʾƼ� �ش� �κ��� ��������
					// viewlist�ȿ� ���� �ִ� ��쿡�� spawn ������ �˷�����
					// ���� ���� ���� ������ ī�޶�ũ�⿡ ���� ���ǰ� ���� �ȵǾ�������, ��� ���� �𸣱⿡ �׳� ��� �����Ѱ� Ŭ���̾�Ʈ���� �ѱ�
					// viewlist �ȿ� ������ Ŭ���̾�Ʈ�� ���� ������ ����

					if (NCDis_check(cl._id, b_zombie3[i]) == false)	// �Ÿ� �ָ� ���� ������ �����Ƿ�... continue
						continue;

					cl.list_lock.lock();
					cl.zombie_list.insert(i);
					cl.list_lock.unlock();

					Send_viewlist_put_packet(cl._id, i, map_type, b_zombie3[i].zombie->GetX(), b_zombie3[i].zombie->GetZ(), MsgType::SC_ZOMBIE_SPAWN, b_zombie3[i].zombie->_type);
				}
				//cout << i << "��° ���� ���� x : " << b_zombie3[i].zombie->GetX() << ", z : " << b_zombie3[i].zombie->GetZ() << "�� �����Ͽ����ϴ� \n";
				AddTimer(i, EVENT_TYPE::EVENT_NPC_MOVE, 667);
			}
			else
			{
				cout << "�̹� �����Ǿ� �ְų� �׾� �ֽ��ϴ�.(�׷� �� ���µ�?) \n";
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
	// 
	//g_clients[c_id].size_lock.lock();
	//memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	//g_clients[c_id]._zombie_prev_size += packet.size;
	//g_clients[c_id].size_lock.unlock();

	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if (g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
}

void Server::Send_zombie_dead_packet(int c_id, int z_id, MapType m_type)
{
	sc_zombie_dead_packet packet;
	packet.id = z_id;
	packet.m_type = m_type;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_DEAD;
	//g_clients[c_id].do_send(sizeof(packet), &packet);
	
	//g_clients[c_id].size_lock.lock();
	//memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	//g_clients[c_id]._zombie_prev_size += packet.size;
	//g_clients[c_id].size_lock.unlock();

	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if (g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
}

void Server::Send_zombie_all_kill_packet(int c_id, MapType m_type)
{
	sc_zombie_all_kill_packet packet;
	packet.m_type = m_type;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_ALL_KILL;
	g_clients[c_id].do_send(sizeof(packet), &packet);
}

void Server::ZombieAllKill(NPC& npc)
{
	npc._state = ZombieState::DEAD;
	npc.zombie->astar.Delete();

	delete npc.zombie;
	npc.zombie = nullptr;

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
		
		//	Send_viewlist_remove_packet(cl._id, npc._id, map_type);
		}
	}
}

void Server::ZombieDead(NPC& npc, MapType m_type)
{
	npc._state = ZombieState::DEAD;
	npc.zombie->astar.Delete();
	remain_zombie_num--;

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

			//Send_viewlist_remove_packet(cl._id, npc._id, map_type);
		}
		Send_zombie_number_packet(cl._id, remain_zombie_num);
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

	//g_clients[c_id].size_lock.lock();
	//memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	//g_clients[c_id]._zombie_prev_size += packet.size;
	//g_clients[c_id].size_lock.unlock();

	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if (g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
}

void Server::ZombiePlayerAttack(NPC& npc, MapType m_type)
{
	if (npc._state != ZombieState::SPAWN)
		return;

	// �� Ŭ���̾�Ʈ���� ���� ���� ��� �϶�� ����
	for (auto& cl : g_clients)
	{
		if (cl._state != ClientState::INGAME)	continue;
		if (cl.map_type != map_type) continue;

		cl.list_lock.lock();
		if (cl.zombie_list.count(npc._id) != 0)
		{
			cl.list_lock.unlock();
			Send_zombie_attack_packet(cl._id, npc._id, m_type);
		}
		else
		{
			cl.list_lock.unlock();
			continue;
		}

		// ���� ������ �Ǵ� �� Ȯ��
		bool attack_check = ZombieAttackRangeCheck(npc.zombie->zombie_dir, npc.zombie->attRange, npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);

		// �����ϸ� ���� Ŭ���̾�Ʈ �˻�
		if (false == attack_check) continue;

		//cout << (int)npc._id << "�� �������� " << (int)cl._id << "�� hp�� " << (int)cl.player->hp << "����";

		// ���������� �ش� Ŭ���� hp�� npc�� ��������ŭ ����
		cl.player->hp -= npc.zombie->damage;

		//cout << (int)cl.player->hp << "�� �����Ͽ����ϴ� \n";

		// �÷��̾��� hp�� 0�̰ų� �Ʒ��� �Ǹ� �÷��̾ �׾��ٰ� �޽��� ����
		if (cl.player->hp <= 0)
		{
			cout << (int)cl._id << "�� �׾����ϴ� \n";

			for (auto& d_cl : g_clients)
			{
				if (d_cl._state != ClientState::INGAME) continue;

				Send_player_dead_packet(d_cl._id, cl._id);
			}

			// �÷��̾� ���º�ȭ
			cl._state = ClientState::DEAD;
			cl.zombie_list.clear();

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

		// ���� �÷��̾��� hp ���� ��ο��� ������
		for (auto& h_cl : g_clients)
		{
			if (h_cl._state != ClientState::INGAME)	continue;

			Send_player_info_packet(h_cl._id, cl._id, cl.player->hp);
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

	AddTimer(z_id, EVENT_TYPE::EVENT_NPC_MOVE, 2000);
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
			AddTimer(c_id, EVENT_TYPE::EVENT_NPC_SEND, 50);

			if (g_clients[c_id]._zombie_prev_size == 0)
			{
				delete exp_over;
				exp_over = nullptr;

				break;
			}

			ZombieSend();
			delete exp_over;
			exp_over = nullptr;
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
			cout << "�̰� �ƹ��͵� �ƴ� \n";
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

	//g_clients[c_id].size_lock.lock();
	//memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	//g_clients[c_id]._zombie_prev_size += packet.size;
	//g_clients[c_id].size_lock.unlock();

	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if (g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
}

void Server::Send_zombie_arrive_packet(int c_id, int z_id, MapType m_type, Direction dir)
{
	sc_zombie_arrive_packet packet;
	packet.id = z_id;
	packet.map_type = m_type;
	packet.size = sizeof(packet);
	packet.type = MsgType::SC_ZOMBIE_ARRIVE;
	packet.dir = dir;
	
	//g_clients[c_id].size_lock.lock();
	//memcpy(g_clients[c_id].zombie_send_buf + g_clients[c_id]._zombie_prev_size, &packet, sizeof(packet));
	//g_clients[c_id]._zombie_prev_size += packet.size;
	//g_clients[c_id].size_lock.unlock();

	bool send = ZombieSendInsert(c_id, &packet, sizeof(packet));
	if (send == false)
	{
		while (1)
		{
			if (g_clients[c_id]._zombie_send_overflow == false)
				send = ZombieSendInsert(c_id, &packet, sizeof(packet));
			if (send) break;
		}
	}
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

void Server::ZombieAstarMove(NPC& npc, MapType m_type)
{
	auto s_time = chrono::system_clock::now();

	// ���� SPAWN ���°� �ƴ϶�� ������ �� ������ ���ư���
	if (npc._state != ZombieState::SPAWN)
	{
		return;
	}

	// Astar �˰����� ������ �ʾҴٸ� npc�� Astar �˰��� ������ ���� ���� �ֱ�
	if (npc.astar_check == false)
	{
		//cout << "���� astar�� ���� ���̴� \n";
		float root_path[3] = { 999999, 999999, 999999 };

		// �� Ŭ���̾�Ʈ���� �Ÿ� ���ϱ�
		for (auto& cl : g_clients)
		{
			if (cl._state != ClientState::INGAME) continue;

			if (cl.map_type != m_type) continue;

			root_path[cl._id] = Distance(npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);
		}

		// npc�� �ǹ̻� ���� ��ǥ(?) ���ϱ�
		int row = (int)npc.zombie->GetZ();
		int col = (int)npc.zombie->GetX();

		float row_result = npc.zombie->GetZ() - row;
		float col_result = npc.zombie->GetX() - col;

		if (row_result > 0.5f)
			row += 1;
		if (col_result > 0.5f)
			col += 1;

		// �Ÿ��� ���� ����� Ŭ���̾�Ʈ�� �������� �ϴ� Astar ���ϱ� 
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
			//	npc.zombie->astar.Delete();
			}
		}
		else
		{
			cout << "�� ������ �ƹ��� �����Ƿ� �ϴ� ���� \n";
			npc._state = ZombieState::SLEEP;
			return;
		}
	}

	if (npc.zombie->root.size() == 1)
	{
		npc.zombie->astar.Delete();
		npc.astar_check = false;
	}

	if (npc.astar_check)
	{
		// ���ǵ�� ������ ���ǵ�
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

		// Astar�� ��� �ִ´ٸ� ������ ���� �ǹǷ� else , ���� �ʾҴٸ� true �̹Ƿ� �̵�
		if (npc.zombie->root.empty() == false)
		{
			// ���� Astar ��� �̵��ϴµ� �̵��ϴ� ������ ���� �浹�� �ߴ��� �����ߴ��� �����ߴ��� �ľ�
			move_check = npc.zombie->Move(z_speed, map);

			// �浹�ϰų� �����ߴٸ� �ٽ� Astar �˰����� ��������
			if (move_check != MoveResult::MOVE)
			{
				npc.astar_check = false;
			//	npc.zombie->astar.Delete();
			}
		}
		else
		{
			npc.astar_check = false;
			npc.zombie->astar.Delete();
		}

		// Astar��� �̵��� ����
		if (move_check == MoveResult::MOVE)
		{
			float root_path[3] = { 999999, 999999, 999999 };

			// �� Ŭ���̾�Ʈ���� ��� ���� �̵��ߴ��� ������
			for (auto& cl : g_clients)
			{
				if (cl._state != ClientState::INGAME) continue;

				if (cl.map_type != m_type) continue;

				// �÷��̾�� ������ �Ÿ����ϱ�
				root_path[cl._id] = Distance(npc.zombie->GetX(), npc.zombie->GetZ(), cl.player->x, cl.player->z);

				if (NCDis_check(cl._id, npc) == true) // �丮��Ʈ�� ���ø��� �Ÿ��� �����Ѵٸ�
				{
					if (prev_zombie_list.count(cl._id) == 0)	// �ٵ� ���� �丮��Ʈ�� ���� ������? �߰�!
					{
						cl.list_lock.lock();
						cl.zombie_list.insert(npc._id);
						cl.list_lock.unlock();

						Send_viewlist_put_packet(cl._id, npc._id, m_type, npc.zombie->GetX(), npc.zombie->GetZ(), MsgType::SC_ZOMBIE_MOVE, npc.zombie->_type);
					}

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
						break;
					}
					case Direction::UP_RIGHT:
					{
						t_x = 0.7f;
						t_z = 0.7f;
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
						break;
					}
					case Direction::DOWN_RIGHT:
					{
						t_x = 0.7f;
						t_z = -0.7f;
						break;
					}
					}

					//m_speed *= 100;

					// ���� �̵� ����, �̵� �� ��ġ, �̵� �ӵ� �� ������
					// (����)������: �̵� �� ��ġ -> �̵� �� ��ġ���� ����, speed�� m_speed -> npc.zombie->speed(������ �ʴ� �̵��ӵ�)�� ����
					Send_zombie_move_packet(cl._id, npc._id, npc.zombie->GetX(), npc.zombie->GetZ(), m_type, t_x, t_z, npc.zombie->speed, npc.zombie->zombie_dir);

					// �̵��� �����߰�, ��ġ ���� �� �÷��̾ �����ߴٸ� �ٽ� A*�� ������, ���̻� ��ġ���� �ʱ�
					if (npc.search_check == false)
					{
						//�÷��̾���� �Ÿ��� ��ġ���� ���� �����ߴٸ� ����� �÷��̾ ���� �ѹ� �� A* ���� �����ϰ� ��
						if (root_path[cl._id] <= npc.zombie->searchRange)
						{
							//npc.search_lock.lock();
							npc.astar_check = false;
							npc.zombie->astar.Delete();
							npc.search_check = true;
							//npc.search_lock.unlock();

							// ��ġ �������� �������Ƿ� ã�Ҵٰ� ������
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
					if (prev_zombie_list.count(cl._id) != 0)		// ���� �丮��Ʈ�� ���� �������� ���� �丮��Ʈ�� �ִܰ� �丮��Ʈ ������ �������� ���̹Ƿ� ����
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

		// �� Ŭ���̾�Ʈ���� �Ÿ� ���ϱ�
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

	// �̵��� ����������, �ֺ��� �÷��̾�� �����Ͽ� �Ǵ� �߰��Ͽ� �÷��̾ ����� ������ �� �ִ� �Ÿ��� �Ǿ����� Ȯ��
	if (npc.attack_delay_time < chrono::system_clock::now())
	{
		for (int i = 0; i < 3; ++i)
		{
			if (g_clients[i]._state != ClientState::INGAME) continue;
			if (g_clients[i].map_type != map_type) continue;

			// �Ÿ��� ������ ���ݹ��� ���� ���ٸ�?
			if (Distance(g_clients[i].player->x, g_clients[i].player->z, npc.zombie->GetX(), npc.zombie->GetZ()) <= npc.zombie->attRange)
			{
				AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_ATTACK, 1);
				return;
			}
		}

		npc.attack_delay_time += 10ms;
	}

	auto e_time = chrono::system_clock::now() - s_time;

	int p_time = 100 - chrono::duration_cast<chrono::milliseconds>(e_time).count();

	// 30fps�� �°� 1�ʿ� 10�� �̵�
	AddTimer(npc._id, EVENT_TYPE::EVENT_NPC_MOVE, p_time);
}

void Server::ZombieMove(int z_id)
{
	switch (map_type)
	{
	case MapType::FIRST_PATH:
	{
		ZombieAstarMove(r_zombie1[z_id], MapType::FIRST_PATH);

		break;
	}
	case MapType::SECOND_PATH:
	{
		ZombieAstarMove(r_zombie2[z_id], MapType::SECOND_PATH);

		break;
	}
	case MapType::FINAL_PATH:
	{
		ZombieAstarMove(r_zombie3[z_id], MapType::FINAL_PATH);

		break;
	}
	case MapType::CHECK_POINT_ONE:
	{
		ZombieAstarMove(b_zombie1[z_id], MapType::CHECK_POINT_ONE);

		break;
	}
	case MapType::CHECK_POINT_TWO:
	{
		ZombieAstarMove(b_zombie2[z_id], MapType::CHECK_POINT_TWO);

		break;
	}
	case MapType::CHECK_POINT_FINAL:
	{
		ZombieAstarMove(b_zombie3[z_id], MapType::CHECK_POINT_FINAL);

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

	cout << "initialize complete \n";

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