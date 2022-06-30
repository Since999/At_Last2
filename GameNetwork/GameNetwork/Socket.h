#pragma once

#include "Expover.h"
#include "Exception.h"

class Socket
{
public :
	SOCKET s_socket;
	char msg[MAX_BUFFER_SIZE];
	HANDLE h_iocp;
	Exp_Over _recv_over;
	Exp_Over* ex_over;
	Exp_Over _send_over;

public:
	Socket();
	virtual ~Socket();

	void Init();
	void Bind();
	void Listen();
	void Connect(const char* const ServerAddress, short ServerPort);
	void CreatePort();
	void OnlyCreatePort(SOCKET& c_socket, int id);
	int Accept_Ex(Exp_Over& exp_over);
	int Accept_Ex_IO(SOCKET& c_socket, Exp_Over& exp_over);

	HANDLE& ReturnHandle();

	void do_send(int num_bytes, void* mess);
	void do_send(int num_bytes);
	void do_recv(int _prev_size);
	void do_recv();
};