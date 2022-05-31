#pragma once

#include "AS_Node.h"

const int MAX_HEAP_LEN = 1200;

struct HeapElement
{
	AS_Node* node;
	float total_cost;
};

class MinHeap
{
private:
	int SaveDataNum;
	HeapElement Heap[MAX_HEAP_LEN];

	mutex heap_lock;

public:
	MinHeap() {
		SaveDataNum = 0;
	};
	~MinHeap() {};

	void Init() { SaveDataNum = 0; };
	void Insert(AS_Node* node, float cost);
	AS_Node* Delete();
	bool Empty() {
		if (SaveDataNum == 0)
			return true;
		else
			return false;
	};
	void Update(AS_Node* node, float cost);

	int GetParant(int num) { return (num / 2); };
	int GetRChild(int num) { return (num * 2) + 1; };
	int GetLChild(int num) { return (num * 2); };
	int GetHighPriorityChild(int num);

	void QSort(int left, int right);
	int Partition(int left, int right);
	void Change(int row, int high) {
		HeapElement temp;
		temp = Heap[row];
		Heap[row] = Heap[high];
		Heap[high] = temp;
	};
};