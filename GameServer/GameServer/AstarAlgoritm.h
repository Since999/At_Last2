#pragma once
#include "clientinfo.h"
#include "MinHeap.h"
#include "LinkList.h"

class AStarAI
{
private:
	AS_Node* as_node[WORLD_HEIGHT][WORLD_WIDTH];
	MinHeap open_list;
	LinkList close_list;
	AS_Map as_map;

	int map_width;
	int map_height;
	int start_x;
	int start_y;

	mutex as_lock;

public:
	AStarAI();
	~AStarAI();

	stack<AS_Node*> AstartSearch(int start_x, int start_y, int end_x, int end_y);

	void Init();
	void New_Init();
	void Delete();
	void New_Delete();
	void Close_List_Delete() { close_list.NodeDelete(); };
	void Near_Node_Connect();
	void Map(AS_Map& map) { as_map = map; };
	int Distance(int x1, int x2, int y1, int y2);
	void Set_Map_Width(int width) { map_width = width; };
	void Set_Map_Height(int height) { map_height = height; };
	void Set_Start_X(int x) { start_x = x; };
	void Set_Start_Y(int y) { start_y = y; }
};