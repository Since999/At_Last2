#pragma once

#include "protocol.h"

class Exp_Over {
public:
	WSAOVERLAPPED		_wsa_over;
	IOType						_IOType;
	WSABUF					_wsa_buf;
	unsigned char				_net_buf[MAX_BUFFER_SIZE];
	unsigned char				_send_buf[MAX_BUFFER_SIZE];
	//int								_target;
public:
	Exp_Over(IOType iotype, int num_bytes, void* mess) : _IOType(iotype)
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_send_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_send_buf, mess, num_bytes);
	}

	Exp_Over(IOType iotype) : _IOType(iotype), _net_buf{}, _wsa_buf{}, _wsa_over()
	{

	}

	Exp_Over() :_net_buf{}, _wsa_buf{}, _wsa_over()
	{
		_IOType = IOType::RECV;
	}

	~Exp_Over()
	{
	}
};