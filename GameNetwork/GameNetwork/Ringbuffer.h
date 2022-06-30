#pragma once

class RingBuffer
{
public:
	RingBuffer();
	RingBuffer(int buffer_size);
	~RingBuffer();

	int GetBufferSize();

	int GetUseSize();
	int GetFreeSize();

	int DirectEnqueueSize();
	int DirectDequeueSize();

	int Enqueue(const unsigned char* buffer, int size);
	int Dequeue(unsigned char* pdest, int size);
	int Peek(unsigned char* pdest, int size);

	void MoveRear(int size);
	void MoveFront(int size);

	unsigned char* GetReadPtr() 
	{
		return read_pointer;
	};
	unsigned char* GetWritePtr()
	{
		return write_pointer;
	};
private:
	unsigned char* begin;
	unsigned char* end;
	unsigned char* read_pointer;
	unsigned char* write_pointer;

	int ring_buffer_size;
};