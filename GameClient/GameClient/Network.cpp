#include "Network.h"

Socket Network::_socket;
array<Client, MAX_PLAYER> Network::g_client;
int Network::_prev_size;
int Network::_id;
bool Network::select_complete;
bool Network::login_complete;
bool Network::game_start;
chrono::milliseconds Network::total_time;

Network::Network()
{
	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	int retval = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (retval != 0)
		throw Exception("Start Fail");

	_socket.Init();
	_socket.CreatePort();
	_socket.Connect(SERVER_IP, SERVER_PORT);
}

Network::~Network() 
{

}

void Network::Initialize()
{
	login_complete = false;
	game_start = false;
}

void Network::Send_request_packet(MsgType type)
{
	cs_request_packet packet;
	packet.size = sizeof(packet);
	packet.type = type;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::ProcessPacket(char* ptr)
{
	char packet_type = ptr[2];

	switch (packet_type)
	{
	case (int)MsgType::SC_LOGIN_OK:
	{
		sc_login_ok_packet* packet = reinterpret_cast<sc_login_ok_packet*>(ptr);

		_id = packet->id;
		g_client[_id]._id = packet->id;
		strcpy_s(g_client[_id].Name, packet->name);

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
		g_client[id]._id = packet->id;
		strcpy_s(g_client[id].Name, packet->name);
		break;
	}
	case (int)MsgType::SC_BARRICADE_SEND:
	{
		game_start = true;
		sc_barricade_packet* packet = reinterpret_cast<sc_barricade_packet*>(ptr);

		for (int i = 0; i < 42; ++i)
			cout << "첫번 째 " << i << " : " << packet->one_base[i].x << ", " << packet->one_base[i].z << "\n";

		for (int i = 0; i < 32; ++i)
			cout << "두번 째 " << i << " : " << packet->two_base[i].x << ", " << packet->two_base[i].z << "\n";

		for (int i = 0; i < 30; ++i)
			cout << "세번 째 " << i << " : " << packet->three_base[i].x << ", " << packet->three_base[i].z << "\n";

		for (int i = 0; i < 30; ++i)
			cout << "네번 째 " << i << " : " << packet->three_base2[i].x << ", " << packet->three_base2[i].z << "\n";

		Send_request_packet(MsgType::CS_GAME_START_REQUEST);
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
		Send_request_packet(MsgType::CS_GAME_START);
		break;
	}
	case (int)MsgType::SC_GAME_START_FAIL:
	{
		// 버퍼링 또는 로딩중 표시.
		// 게임 시작했는지에 대한 타이머 질문에 대한 서버에서 아직 안됬다고 대답

		cout << "게임을 시작하는데 실패하였습니다. \n";
		break;
	}
	case (int)MsgType::SC_PLAYER_ATTACK:
	{
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
	case (int)MsgType::SC_PLAYER_HIDE:
	{
		break;
	}
	case (int)MsgType::SC_PLAYER_INFECTION:
	{
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

		// row, col 은 문의 중앙 위치
		// size_x 와 size_z는 가로 세로의 절반 길이

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

		g_client[packet->id].x = packet->x;
		g_client[packet->id].z = packet->z;
		g_client[packet->id]._state = packet->state;

		cout << "client [" << g_client[packet->id]._id << "] 의 x : " << g_client[packet->id].x << ", z : " << g_client[packet->id].z << "\n";
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

		g_client[packet->id]._type = packet->playertype;
		g_client[packet->id].hp = packet->hp;
		g_client[packet->id].maxhp = packet->maxhp;
		g_client[packet->id].shp = packet->shp;
		g_client[packet->id].maxshp = packet->maxshp;
		g_client[packet->id].infection = packet->infection;
		g_client[packet->id].x = packet->x;
		g_client[packet->id].z = packet->z;
		g_client[packet->id]._state = packet->state;

		break;
	}
	case (int)MsgType::SC_PLAYER_SELECT_FAIL:
	{
		// 플레이어가 캐릭터 선택에 실패했다는 메시지 출력이 있으면 좋을 것 같음
		cout << "플레이어가 이미 선택되었습니다 \n";
		select_complete = false;
		break;
	}
	case (int)MsgType::SC_PLAYER_SPECIAL:
	{
		break;
	}
	case (int)MsgType::SC_PLAYER_SEARCH:
	{
		sc_search_packet* packet = reinterpret_cast<sc_search_packet*>(ptr);

		cout << "x : " << packet->x << ", z : " << packet->z << "에 " << (char)packet->obj_type <<  "이 있습니다. \n";
		break;
	}
	case (int)MsgType::SC_PLAYER_SEARCH_FAIL:
	{
		cout << "이곳 주위에는 문이 존재하지 않습니다 \n";
		break;
	}
	case (int)MsgType::SC_UPDATE_OBJECT_INFO:
	{
		break;
	}
	case (int)MsgType::SC_UPDATE_PLAYER_INFO:
	{
		break;
	}
	case (int)MsgType::SC_UPDATE_STATE:
	{
		break;
	}
	case (int)MsgType::SC_UPDATE_ZOMBIE_INFO:
	{
		break;
	}
	case (int)MsgType::SC_WIN_STATE:
	{
		break;
	}
	case (int)MsgType::SC_ZOMBIE_ATTACK:
	{
		break;
	}
	case (int)MsgType::SC_ZOMBIE_MAD:
	{
		break;
	}
	case (int)MsgType::SC_ZOMBIE_MOVE:
	{
		break;
	}
	case (int)MsgType::SC_ZOMBIE_SPAWN:
	{
		break;
	}
	case (int)MsgType::SC_WAIT:
	{
		
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

void Network::ProcessData(char* net_buf, int size)
{
	int remain_data = size + _prev_size;
	char* packet_start = net_buf;
	int packet_size = ((int)packet_start[1]*256)+(int)packet_start[0];

	while (packet_size <= remain_data)
	{
		ProcessPacket(packet_start);
		remain_data -= packet_size;
		packet_start += packet_size;
		if (remain_data > 0)
			packet_size = packet_start[0];
		else { 
			if ((((int)packet_start[1] * 256) + (int)packet_start[0]) != 0)
			{
				packet_size = ((int)packet_start[1] * 256) + (int)packet_start[0];
				remain_data += packet_size;
			}
			else {
				ZeroMemory(&_socket._recv_over._net_buf, sizeof(_socket._recv_over._net_buf));
				break;
			}
		}
	}

	if (remain_data > 0)
	{
		_prev_size = remain_data;
		memcpy(&net_buf, packet_start, remain_data);
	}
}

void Network::ClientMain()
{
	if (false == _socket.do_recv(_prev_size))
	{
		cout << "recv error \n";
	}
	else
	{
		ProcessData((char*)_socket._recv_over._net_buf, (_socket._recv_over._net_buf[1] * 256) + _socket._recv_over._net_buf[0]);
	}

	// 위에서 처리한대로 여기서 화면을 그림(다렉부분)

}

void Network::send_player_select_packet(PlayerType type)
{
	cs_select_packet packet;
	packet.playertype = type;
	packet.size = sizeof(packet);
	packet.type = MsgType::CS_PLAYER_SELECT;
	_socket.do_send(sizeof(packet), &packet);
}

void Network::send_player_move_packet(Direction dir, PlayerState state)
{
	cs_move_packet packet;
	packet.dir = dir;
	packet.playerstate = state;
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

void Network::GetKey()
{
	while (true) {
		if (total_time > 1000ms)
			break;
		if (GetAsyncKeyState(VK_RIGHT) & 0x0001) { //오른쪽
			if (GetAsyncKeyState(VK_UP) & 0x8001)							send_player_move_packet(Direction::UP_RIGHT, PlayerState::WALK);
			else if (GetAsyncKeyState(VK_DOWN) & 0x8001)				send_player_move_packet(Direction::DOWN_RIGHT, PlayerState::WALK);
			else																				send_player_move_packet(Direction::RIGHT, PlayerState::WALK);

			break;
		}
		else if (GetAsyncKeyState(VK_LEFT) & 0x0001) { //왼쪽
			if (GetAsyncKeyState(VK_UP) & 0x8001)							send_player_move_packet(Direction::UP_LEFT, PlayerState::WALK);
			else if (GetAsyncKeyState(VK_DOWN) & 0x8001)				send_player_move_packet(Direction::DOWN_LEFT, PlayerState::WALK);
			else																				send_player_move_packet(Direction::LEFT, PlayerState::WALK);

			break;
		}
		else if (GetAsyncKeyState(VK_UP) & 0x0001) { //위
			if (GetAsyncKeyState(VK_LEFT) & 0x8001)							send_player_move_packet(Direction::UP_LEFT, PlayerState::WALK);
			if (GetAsyncKeyState(VK_RIGHT) & 0x8001)						send_player_move_packet(Direction::UP_RIGHT, PlayerState::WALK);
			else																				send_player_move_packet(Direction::UP, PlayerState::WALK);

			break;
		}
		else if (GetAsyncKeyState(VK_DOWN) & 0x0001) { //아래
			if (GetAsyncKeyState(VK_LEFT) & 0x8001)						send_player_move_packet(Direction::DOWN_LEFT, PlayerState::WALK);
			else if (GetAsyncKeyState(VK_RIGHT) & 0x8001)				send_player_move_packet(Direction::DOWN_RIGHT, PlayerState::WALK);
			else																				send_player_move_packet(Direction::DOWN, PlayerState::WALK);

			break;
		}
		else if (GetAsyncKeyState('E') & 0x0001)
		{
			Send_request_packet(MsgType::CS_PLAYER_INTERATION);
		}
		else if (GetAsyncKeyState('M') & 0x0001)
		{
			char chat_msg[MAX_CHAT_SIZE];
			ZeroMemory(chat_msg, sizeof(chat_msg));
			total_time = 0ms;
			cin >> chat_msg;
			Send_chat_packet(chat_msg);
			cout << "chat : " << chat_msg << "\n";
		}
	}
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

		ClientMain();

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
		total_time = 0ms;
		Exp_Over* exp_over = reinterpret_cast<Exp_Over*>(p_over);
		if (ret == SOCKET_ERROR)
			throw Exception("Queue Error");

		switch (exp_over->_IOType)
		{
		case IOType::RECV:
			ClientMain();

			if(select_complete && game_start)
				GetKey();
			break;
		case IOType::SEND:
			if (num_byte != exp_over->_wsa_buf.len) {
				exit(0);
			}
			delete exp_over;
			break;
		default:
			cout << "이건 아무것도 아님 \n";

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

			send_player_select_packet(type);
			this_thread::sleep_for(10ms);

			Send_request_packet(MsgType::CS_BARRICADE_REQUEST);
			this_thread::sleep_for(10ms);
		}

	}
}

void Network::Do_Timer()
{
	while (true)
	{
		this_thread::sleep_for(10ms);
		
		total_time += 10ms;
		if (total_time > 1000ms)
		{
			if (game_start == false)
			{
				Send_request_packet(MsgType::CS_BARRICADE_REQUEST);
				this_thread::sleep_for(10ms);
			}

			Send_request_packet(MsgType::CS_SERVER_REQUEST);

			total_time = 0ms;
		}
	}
}

void Network::Update() 
{
	thread Timer_thread{ Do_Timer };

	for (int i = 0; i < 1; ++i)
		worker_threads.emplace_back(Work);

	Timer_thread.join();

	for (auto& th : worker_threads) {
		th.join();
	}
	WSACleanup();
}