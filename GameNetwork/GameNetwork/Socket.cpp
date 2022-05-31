#include "Socket.h"

Socket::Socket() : s_socket(), msg{}, _recv_over{ IOType::RECV }, h_iocp(), ex_over{}
{

}

Socket::~Socket() 
{
	if(s_socket)
		closesocket(s_socket);
}

void Socket::Init() 
{
	s_socket = WSASocket(AF_INET, SOCK_STREAM,0, NULL, 0, WSA_FLAG_OVERLAPPED);
}

void Socket::Bind() 
{
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int retval = bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR)
		throw Exception("Bind Fail");
}

void Socket::Listen() 
{
	if (listen(s_socket, SOMAXCONN) != 0)
		throw Exception("Listen Fail");
}

void Socket::CreatePort() 
{
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), h_iocp, 0, 0);
}

void Socket::OnlyCreatePort(SOCKET& c_socket, int id)
{
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), h_iocp, id, 0);
}

void Socket::Connect(const char* const ServerAddress, short ServerPort)
{
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, ServerAddress, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(ServerPort);
	int retval = WSAConnect(s_socket, (SOCKADDR*)&serveraddr, sizeof(serveraddr), NULL, NULL, NULL, NULL);
	if (retval == SOCKET_ERROR) {
		throw Exception("connect failed");
	}
}


int Socket::Accept_Ex(Exp_Over& exp_over) 
{
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	*(reinterpret_cast<SOCKET*>(&exp_over._net_buf)) = c_socket;
	ZeroMemory(&exp_over._wsa_over, sizeof(exp_over._wsa_over));
	exp_over._IOType = IOType::ACCEPT;

	int retval = AcceptEx(s_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &exp_over._wsa_over);
	if (retval == INVALID_SOCKET)
		throw Exception("Accept Fail");

	return retval;
}

int Socket::Accept_Ex_IO(SOCKET& c_socket,Exp_Over& exp_over)
{
	cout << "loading \n";

	ZeroMemory(&exp_over._wsa_buf, sizeof(exp_over._wsa_over));
	c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	*(reinterpret_cast<SOCKET*>(exp_over._net_buf)) = c_socket;
	int retval = AcceptEx(s_socket, c_socket, exp_over._net_buf + 8, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &exp_over._wsa_over);

	return retval;
}

void Socket::do_recv(int _prev_size)
{
	DWORD recv_flag = 0;
	ZeroMemory(&_recv_over._wsa_over, sizeof(_recv_over._wsa_over));
	_recv_over._wsa_buf.buf = reinterpret_cast<char*>(_recv_over._net_buf + _prev_size);
	_recv_over._wsa_buf.len = sizeof(_recv_over._net_buf) - _prev_size;
	
	int ret = WSARecv(s_socket, &_recv_over._wsa_buf, 1, 0, &recv_flag, &_recv_over._wsa_over, NULL);
	if (SOCKET_ERROR == ret) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			throw Exception("Recv Fail");
		}
	}
}

void Socket::do_send(int num_bytes, void* mess)
{
	ex_over = new Exp_Over(IOType::SEND, num_bytes, mess);
	int ret = WSASend(s_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
	if (SOCKET_ERROR == ret) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
			throw Exception("Send Fail");
	}
}

HANDLE& Socket::ReturnHandle()
{
	return h_iocp;
}