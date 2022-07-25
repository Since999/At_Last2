#pragma once

#include "common.h"
#include <filesystem>

template <class T>
std::pair<int, int> mul(const std::pair<int, int>& a, const T& b);

std::pair<int, int> lerp(const std::pair<int, int>& a, const std::pair<int, int>& b, double t);
std::pair<int, int> Plus(const std::pair<int, int>& a, const std::pair<int, int>& b);

#define ENABLE_BEZIER
#define ENABLE_MULTIPLE_

const int MAP_SIZE = 120;
const int ROAD_SIZE = 24;

const iPos Start_Map_Pos = { 0,0 };
const iPos One_Base_Pos = { 199,238 };
const iPos One_Base_End_Pos = { 319, 358 };
const iPos TWO_Base_Pos = { 510,275 };
const iPos TWO_Base_End_Pos = { 628, 395 };
const iPos THREE_Base_Pos = { 821,275 };
const iPos THREE_Base_End_Pos = { 939,395 };
const iPos THREE_Base_Pos2 = { 940, 275 };
const iPos THREE_Base_End_Pos2 = { 1053, 395 };

const iPos One_Road_Pos = { 34, 85 };
const iPos One_Road_Pos2 = { 34, 133 };
const iPos One_Road_Pos3 = { 199, 133 };
const iPos Two_Road_Pos = { 321, 322 };
const iPos Three_Road_Pos = { 631, 322 };

const iPos Exit_Pos = { 1053, 335 };
const iPos Exit_End_Pos = { 1098, 355 };

const int One_Level = 42;
const int Two_Level = 32;
const int Three_Level = 30;

enum class AS_MAP_TYPE : char
{
	ROAD,
	WALL,
	START,
	END,
	PASS_ROAD
};

class Maze
{
private:
	int arr[MAP_SIZE][MAP_SIZE];
	std::pair<int, int> entrance, exit;
	const int dpp = 10;

	random_device rd;
	default_random_engine dre{ rd() };
public:
	Maze();
	~Maze();

	void GenerateMap(int num = 2);
	void Reset(int num = 2);

	void GenerateRoad(int dot = 10);
	int bezier(const std::vector<std::pair<int, int>>& dots, double pos);

	int get(int a, int b) { return arr[a][b]; }
};

struct MazeInfo
{
	char mapdata;
	bool check;
};

class Map
{
public:
	Maze maze;
	char map[WORLD_HEIGHT][WORLD_WIDTH];
	MazeInfo temp_map[MAP_SIZE][MAP_SIZE];
	iPos* barricade_pos;
	MapInfo map_info;

	iPos one_base[One_Level];
	iPos two_base[Two_Level];
	iPos three_base[Three_Level];
	iPos three_base2[Three_Level];
public:
	Map();
	~Map();

	void ReadMapFile();
	void MakeMaze(iPos pos, int barricade, iPos* base, iPos end_pos);
	void Initialize();
	void ChangeWall(iPos pos, ANGLE angle);
	void InputDoor(float row, float col, float size_x, float size_z, DIR dir, int num);
	void MakeDoor();
	bool CheckBarricade(iPos pos, ANGLE angle);
	void EditMap(iPos sp, iPos ep);
	void MazeTypeInit(iPos* base, BarricadeType b_type, int num);
};

class AS_Map
{
private:
	AS_MAP_TYPE as_map[WORLD_HEIGHT][WORLD_WIDTH];

public:
	AS_Map();
	~AS_Map();

	void InitMap(Map& map);
	void EditMap(int s_x, int s_z, int e_x, int e_z, Map& map);
	AS_MAP_TYPE GetMapInfo(int x, int y) { return as_map[y][x]; };
};