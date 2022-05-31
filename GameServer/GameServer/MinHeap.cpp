#include "MinHeap.h"

void MinHeap::Insert(AS_Node* node, float cost)
{
	//heap_lock.lock();
	int index = SaveDataNum + 1;
	//heap_lock.unlock();
	HeapElement elem = { node, cost };

	while (index != 1)
	{
		if (elem.total_cost < Heap[GetParant(index)].total_cost)
		{
			//heap_lock.lock();
			Heap[index] = Heap[GetParant(index)];
			index = GetParant(index);
			//heap_lock.unlock();
		}
		else
			break;
	}

	//heap_lock.lock();
	Heap[index] = elem;
	SaveDataNum += 1;
	///heap_lock.unlock();
}

AS_Node* MinHeap::Delete()
{
	HeapElement data = Heap[1];
	HeapElement elem = Heap[SaveDataNum];

	int parent_index = 1;
	int child_index;

	while (child_index = GetHighPriorityChild(parent_index))
	{
		if (elem.total_cost < Heap[child_index].total_cost)
			break;

		//heap_lock.lock();
		Heap[parent_index] = Heap[child_index];
		//heap_lock.unlock();

		parent_index = child_index;
	}

	//heap_lock.lock();
	Heap[parent_index] = elem;
	SaveDataNum -= 1;
	//heap_lock.unlock();

	return data.node;
}

void MinHeap::Update(AS_Node* node, float cost)
{
	bool check = false;
	int i;

	int result = SaveDataNum;

	for (i = 1; i <= result; ++i)
	{
		if (Heap[i].node == node)
		{
			check = true;
			break;
		}
	}

	if (check)
	{
		//heap_lock.lock();
		Heap[i].total_cost = cost;
		//heap_lock.unlock();
		QSort(1, SaveDataNum);
	}
}

int MinHeap::GetHighPriorityChild(int num)
{
	if (GetLChild(num) > SaveDataNum)
	{
		return 0;
	}
	else if (GetLChild(num) == SaveDataNum)
	{
		return GetLChild(num);
	}
	else
	{
		if (Heap[GetLChild(num)].total_cost < Heap[GetRChild(num)].total_cost)
			return GetLChild(num);
		else
			return GetRChild(num);
	}
}

void MinHeap::QSort(int left, int right)
{
	if (left <= right)
	{
		int Pivot = Partition(left, right);
		QSort(left, Pivot - 1);
		QSort(Pivot+1, right);
	}
}

int MinHeap::Partition(int left, int right)
{
	int low = left + 1;
	int high = right;
	float pivot = Heap[left].total_cost;

	while (low <= high)
	{
		while (pivot >= Heap[low].total_cost && low <= right)
			low++;
		while (pivot <= Heap[high].total_cost && high >= left + 1)
			high--;

		if (low <= high)
			Change(low, high);
	}

	Change(left, high);
	return high;
}