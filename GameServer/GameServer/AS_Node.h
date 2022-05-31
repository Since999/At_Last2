#pragma once

#include <iostream>
#include "map.h"

class AS_Node
{
private:
	AS_Node* _parent_node;			// 부모 노드, 최종적으로는 시작점
	float start_cost;						// 시작 비용
	float end_cost;						// 끝 비용
	float total_dis;						// 총 비용
	AS_MAP_TYPE map_type;
	int x;
	int y;
	bool Open_List;
	bool Close_List;

public:
	AS_Node* near_node[8];

public:
	AS_Node()
	{
		_parent_node = nullptr;
		start_cost = 0;
		end_cost = 0;
		total_dis = 0;
		map_type = AS_MAP_TYPE::ROAD;
		x = 0;
		y = 0;
		Open_List = false;
		Close_List = false;

		for (int i = 0; i < 8; ++i)
			near_node[i] = nullptr;
	};
	~AS_Node() {};

	AS_Node* Get_Parent() { return _parent_node; };
	float Get_Start_Cost() { return start_cost; };
	float Get_End_Cost() { return end_cost; };
	float Get_Total_Distance() { return total_dis; };
	AS_MAP_TYPE Get_Map_Type() { return map_type; };
	int Get_X() { return x; };
	int Get_Y() { return y; };
	bool Get_Open_List() { return Open_List; };
	bool Get_Close_List() { return Close_List; };
	void Set_Parent(AS_Node* node) { _parent_node = node; };
	void Set_Start_Cost(float cost) { start_cost = cost; };
	void Set_End_Cost(float cost) { end_cost = cost; };
	void Set_Total_Distance(float distance) { total_dis = distance; };
	void Set_Total_Distance(float distance, float distance2) { total_dis = distance + distance2; };
	void Set_Map_Type(AS_MAP_TYPE type) {  map_type = type; };
	void Set_X(int pos) { x = pos; };
	void Set_Y(int pos) { y = pos; };
	void Set_Open_List(bool list) { Open_List = list; };
	void Set_Close_List(bool list) { Close_List = list; };
};