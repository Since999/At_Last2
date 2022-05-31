#pragma once

#include "Client.h"

class Network {
public:
	static Socket _socket;
	static array<Client, MAX_PLAYER> g_client;
	static int _id;
	static int _prev_size;
	static bool select_complete;
	static bool login_complete;
	static bool game_start;

	static chrono::milliseconds total_time;

	vector<thread> worker_threads;
public:
	Network();
	~Network();
	
	static void Initialize();
	static void send_login_packet(char* str);
	static void send_player_select_packet(PlayerType type);
	void Update();
	static void ProcessPacket(char* ptr);
	static void ClientMain();
	static void ProcessData(char* net_buf, int size);
	static void Work();
	static void GetKey();
	static void send_player_move_packet(Direction dir, PlayerState state);
	static void Do_Timer();
	static void Send_request_packet(MsgType type);
	static void Send_chat_packet(char* msg);
};