#include "Ringbuffer.h"
#include "protocol.h"

RingBuffer::RingBuffer() : ring_buffer_size(MAX_BUFFER_SIZE)
{
	begin = (unsigned char*)malloc(ring_buffer_size);
	end = begin + ring_buffer_size;
	read_pointer = write_pointer = begin;
}

RingBuffer::RingBuffer(int buffer_size) : ring_buffer_size(buffer_size)
{
	begin = (unsigned char*)malloc(ring_buffer_size);
	end = begin + ring_buffer_size;
	read_pointer = write_pointer = begin;
}

RingBuffer::~RingBuffer()
{
	free(begin);
}

// 링버퍼 사이즈
int RingBuffer::GetBufferSize()
{
	return end - begin - 1;
}

// 현재 사용중인 사이즈
int RingBuffer::GetUseSize()
{
	if (write_pointer >= read_pointer)
	{
		return write_pointer - read_pointer;
	}

	return (write_pointer - begin) + (end - read_pointer - 1);
}

// 현재 사용중이지 않은 사이즈
int RingBuffer::GetFreeSize()
{
	if (write_pointer >= read_pointer)
	{
		return (end - write_pointer) + (read_pointer - begin - 1);
	}

	return read_pointer - write_pointer - 1;
}

// 한번의 memcpy로 뽑아낼 수 있는 사이즈
int RingBuffer::DirectEnqueueSize()
{
	if (write_pointer >= read_pointer)
	{
		return end - write_pointer - 1;
	}

	return read_pointer - write_pointer - 1;
}

// 한번에 dequeue 할 수 있는 사이즈
int RingBuffer::DirectDequeueSize()
{
	if (write_pointer >= read_pointer)
	{
		return write_pointer - read_pointer;
	}

	return end - read_pointer - 1;
}

// 사이즈만큼 넣고 남는 사이즈가 적으면 0 리턴
int RingBuffer::Enqueue(const unsigned char* buffer, int size)
{
	if (GetFreeSize() < size) return 0;

	if (DirectEnqueueSize() >= size)
	{
		memcpy_s(write_pointer, size, buffer, size);
		MoveRear(size);
		return size;
	}
	const unsigned char* temp = buffer;

	int direct_enqueue_size = DirectEnqueueSize();
	memcpy_s(write_pointer, direct_enqueue_size, temp, direct_enqueue_size);
	temp += direct_enqueue_size;
	MoveRear(direct_enqueue_size);

	int remain_size = size - direct_enqueue_size;
	memcpy_s(write_pointer, remain_size, temp, remain_size);
	MoveRear(remain_size);
	return size;
}

// 사이즈만큼 dequeue
int RingBuffer::Dequeue(unsigned char* pdest, int size)
{
	if (GetUseSize() < size) return 0;

	if (DirectDequeueSize() >= size)
	{
		memcpy_s(pdest, size, read_pointer, size);
		MoveFront(size);
		return size;
	}

	unsigned char* p_dest_temp = pdest;
	int direct_dequeue_size = DirectDequeueSize();

	memcpy_s(p_dest_temp, direct_dequeue_size, read_pointer, direct_dequeue_size);
	MoveFront(direct_dequeue_size);

	int remain_size = size - direct_dequeue_size;
	p_dest_temp += direct_dequeue_size;

	memcpy_s(p_dest_temp, remain_size, read_pointer, remain_size);
	MoveFront(remain_size);

	return size;
}

// if서 true 나오면 복사만하고 return 함, 리드 포인터 이동하지 않고 내용만 꺼내줌
int RingBuffer::Peek(unsigned char* pdest, int size)
{
	if (GetUseSize() < size) return 0;

	if (DirectDequeueSize() >= size)
	{
		memcpy_s(pdest, size, read_pointer, size);
		return size;
	}

	unsigned char* p_front_temp = read_pointer;
	unsigned char* p_dest_temp = pdest;
	int direct_dequeue_size = DirectDequeueSize();

	memcpy_s(p_dest_temp, direct_dequeue_size, read_pointer, direct_dequeue_size);
	MoveFront(direct_dequeue_size);

	int remain_size = size - direct_dequeue_size;
	p_dest_temp += direct_dequeue_size;
	
	memcpy_s(p_dest_temp, remain_size, read_pointer, remain_size);
	read_pointer = p_front_temp;

	return size;
}

// 끝,앞으로 이동?
void RingBuffer::MoveRear(int size)
{
	write_pointer += size;

	if (write_pointer >= end - 1)
	{
		int over_flow = write_pointer - end;
		write_pointer = begin + over_flow;
	}
}

void RingBuffer::MoveFront(int size)
{
	read_pointer += size;

	if (read_pointer >= end - 1)
	{
		int over_flow = read_pointer - end;
		read_pointer = begin + over_flow;
	}
}