#include "map.h"

template <class T>
std::pair<int, int> mul(const std::pair<int, int>& a, const T& b)
{
	std::pair<int, int> result;
	result.first = a.first * b;
	result.second = a.second * b;
	return result;
}

std::pair<int, int> lerp(const std::pair<int, int>& a, const std::pair<int, int>& b, double t)
{
	std::pair<int, int> tmp;
	tmp.first = a.first * (1 - t) + b.first * t;
	tmp.second = a.second * (1 - t) + b.second * t;
	return tmp;
}

std::pair<int, int> Plus(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	std::pair<int, int> result;
	result.first = a.first + b.first;
	result.second = a.second + b.second;
	return result;
}

Maze::Maze()
{
	entrance.first = 0;
	entrance.second = 0;
	exit.first = MAP_SIZE - 1;
	exit.second = MAP_SIZE - 1;
}

Maze::~Maze()
{
}

void Maze::GenerateMap(int num)
{
	entrance.first = 0;
	entrance.second = 0;
	exit.first = MAP_SIZE - 1;
	exit.second = MAP_SIZE - 1;
#ifdef ENABLE_MULTIPLE_
	for (int i = 0; i < num; ++i) {
		GenerateRoad();
	}
	entrance.first = 0;
	entrance.second = MAP_SIZE / 2;
	exit.first = MAP_SIZE - 1;
	exit.second = MAP_SIZE / 2;
	for (int i = 0; i < num; ++i)
	{
		GenerateRoad();
	}
	entrance.first = MAP_SIZE - 1;
	entrance.second = MAP_SIZE - 1;
	exit.first = 0;
	exit.second = 0;
	for (int i = 0; i < num; ++i)
	{
		GenerateRoad();
	}
#else
	GenerateRoad();
#endif
}

void Maze::Reset(int num)
{
	for (int i = 0; i < MAP_SIZE; ++i)
		for (int j = 0; j < MAP_SIZE; ++j)
			arr[i][j] = (int)MazeWall::WALL;

	GenerateMap(num);
}

void Maze::GenerateRoad(int dotnum)
{
	std::uniform_int_distribution<int> random_dotnum(MAP_SIZE * (4 / 3) / dpp, MAP_SIZE * 2 / dpp);
	std::uniform_int_distribution<int> random_dot(0, MAP_SIZE);
	if (dotnum > MAP_SIZE) return;
	if (dotnum == 0) dotnum = random_dotnum(dre);
	std::vector<std::pair<int, int>> dots;
	dots.reserve(dotnum);
	dots.push_back(entrance);

	double term = (double)MAP_SIZE / (double)dotnum;
	for (int i = 1; i < dotnum - 1; ++i) {
		dots.push_back({ (int)(term * i), random_dot(dre) });
	}
	dots.push_back(exit);
	dots.push_back(exit);
	dots.push_back(exit);
	dots.push_back(exit);
	dots.push_back(exit);
	// 배지어 알고리즘
#ifdef ENABLE_BEZIER

	double t = 0 / term;
	int pre = bezier(dots, t);
	int bezier_dot;
	arr[0][pre] = (int)MazeWall::ROAD;

	for (int i = 1; i < MAP_SIZE; ++i)
	{
		double t = i / term;
		bezier_dot = bezier(dots, t);
		int differ = (bezier_dot - pre < 0) ? -1 : 1;
		for (; pre != bezier_dot; pre += differ) {
			arr[i][pre] = (int)MazeWall::ROAD;
			arr[i - 1][pre] = (int)MazeWall::ROAD;
		}

		arr[i][bezier_dot] = (int)MazeWall::ROAD;
		pre = bezier_dot;
	}
	for (auto dot : dots) {
		arr[dot.first][dot.second] = (int)MazeWall::DOT;
	}
#else
	for (auto dot : dots) {
		arr[dot.first][dot.second] = (int)MazeWall::ROAD;
	}
#endif
}

int Maze::bezier(const std::vector<std::pair<int, int>>& dots, double pos)
{
	int a = (int)pos;
	if (a < 1) a = 1;
	double t = (pos + 1 - a) / 3;
	auto tmp = mul(dots[a - 1], pow(1 - t, 3));
	tmp = Plus(tmp, mul(dots[a], 3 * pow(1 - t, 2) * t));
	tmp = Plus(tmp, mul(dots[a + 1], 3 * (1 - t) * pow(t, 2)));
	tmp = Plus(tmp, mul(dots[a + 2], pow(t, 3)));
	return tmp.second;
}

Map::Map()
{

}

Map::~Map()
{

}

void Map::ReadMapFile()
{
	ifstream in("no_door_map2.txt");

	for (int i = 0; i < WORLD_HEIGHT; ++i)
		for (int j = 0; j < WORLD_WIDTH; ++j)
			in >> map[i][j];

	in.close();
}

void Map::ChangeWall(iPos pos, bool p)
{
	Object obj;
	if (p)
	{
		for (int j = pos.z - 1; j <= pos.z + 1; ++j)
		{
			for (int i = pos.x - 2; i <= pos.x + 2; ++i)
			{
				map[j][i] = (char)MazeWall::BARRICADE;

				obj.row = i;
				obj.col = pos.z;
				obj.size_x = 1;
				obj.size_z = 1;
				obj.state = ObjectState::NORMAL;
				obj.type = ObjectType::BARRICADE;
				obj.x = (float)i;
				obj.z = pos.z;
				map_info.Barricade.emplace_back(obj);
			}
		}
	}
	else
	{
		for (int j = pos.x - 1; j <= pos.x + 1; ++j)
		{
			for (int i = pos.z - 2; i <= pos.z + 2; ++i)
			{
				map[i][j] = (char)MazeWall::BARRICADE;

				obj.row = pos.x;
				obj.col = i;
				obj.size_x = 1;
				obj.size_z = 1;
				obj.state = ObjectState::NORMAL;
				obj.type = ObjectType::BARRICADE;
				obj.x = pos.x;
				obj.z = (float)i;
				map_info.Barricade.emplace_back(obj);
			}
		}
	}
}

bool Map::CheckBarricade(iPos pos, bool p)
{
	if (p)
	{
		for (int j = pos.z - 1; j <= pos.z + 1; ++j)
		{
			for (int i = pos.x - 2; i <= pos.x + 2; ++i)
			{
				if (map[j][i] == (char)MazeWall::BARRICADE)
					return true;
				if (map[j][i] == (char)MazeWall::WALL)
					return true;
			}
		}
	}
	else
	{
		for (int j = pos.x - 1; j <= pos.x + 1; ++j)
		{
			for (int i = pos.z - 2; i <= pos.z + 2; ++i)
			{
				if (map[i][j] == (char)MazeWall::BARRICADE)
					return true;
				if (map[i][j] == (char)MazeWall::WALL)
					return true;
			}
		}
	}

	return false;
}


void Map::MakeMaze(iPos pos, int barricade, iPos* base, iPos end_pos)
{
	maze.Reset();

	for (int z = 0; z < MAP_SIZE; ++z)
		for (int x = 0; x < MAP_SIZE; ++x) {
			temp_map[z][x].mapdata = maze.get(z, x);
			temp_map[z][x].check = false;
		}

	vector<iPos> barricade_pos;
	iPos temp_pos;
	for (int z = 5; z < end_pos.z - pos.z - 5; ++z) {
		for (int x = 5; x < end_pos.x - pos.x - 5; ++x) {
			if (temp_map[z][x].mapdata != (char)MazeWall::WALL && temp_map[z][x].check == false)
			{
				temp_map[z][x].check = true;
				temp_pos.x = x;
				temp_pos.z = z;
				barricade_pos.emplace_back(temp_pos);
			}
			
			// 이전 바리게이트 위치 생성기
			/*
			if (temp_map[z][x].mapdata == (char)MazeWall::WALL)
			{
				if (temp_map[z - 1][x].check == true || temp_map[z + 1][x].check == true || temp_map[z][x - 1].check == true || temp_map[z][x + 1].check == true)
					continue;
				if (temp_map[z - 1][x].mapdata != (char)MazeWall::WALL)
				{
					temp_map[z - 1][x].check = true;
					temp_pos.x = x;
					temp_pos.z = z;
					barricade_pos.emplace_back(temp_pos);
				}
				if (temp_map[z + 1][x].mapdata != (char)MazeWall::WALL)
				{
					temp_map[z + 1][x].check = true;
					temp_pos.x = x;
					temp_pos.z = z;
					barricade_pos.emplace_back(temp_pos);
				}
				if (temp_map[z][x - 1].mapdata != (char)MazeWall::WALL)
				{
					temp_map[z][x - 1].check = true;
					temp_pos.x = x;
					temp_pos.z = z;
					barricade_pos.emplace_back(temp_pos);
				}
				if (temp_map[z][x + 1].mapdata != (char)MazeWall::WALL)
				{
					temp_map[z][x + 1].check = true;
					temp_pos.x = x;
					temp_pos.z = z;
					barricade_pos.emplace_back(temp_pos);
				}
			}
			*/
		}
	}

	int size = (int)(barricade_pos.size() - 1);

	random_device rd;
	default_random_engine dre{ rd() };
	uniform_int_distribution<> random{ 0, size };

	vector<int> v;
	int random_pos;
	bool same;

	for (int a = 0; a < barricade; ++a)
	{
		random_pos = random(dre);

		same = false;
		for (auto& r : v)
		{
			if (r == random_pos) {
				same = true;
				break;
			}
		}

		if (same) {
			a--;
			continue;
		}

		v.emplace_back(random_pos);
	}

	iPos rPos;
	int t = 0;

	for (auto p : v)
	{
		rPos.x = barricade_pos[p].x + pos.x;
		rPos.z = barricade_pos[p].z + pos.z;

		if (p % 2 == 0) {
			if (CheckBarricade(rPos, false)) {
				while (1) {
					random_pos = random(dre);

					same = false;
					for (auto r : v)
					{
						if (r == random_pos) {
							same = true;
							break;
						}
					}

					if (same) {
						continue;
					}

					rPos.x = barricade_pos[random_pos].x + pos.x;
					rPos.z = barricade_pos[random_pos].z + pos.z;

					if (CheckBarricade(rPos, false))
						continue;

					break;
				}
			}
			ChangeWall(rPos, false);
			base[t].x = rPos.x;
			base[t].z = rPos.z;
			base[t].dir = DIR::HEIGHT;
			t++;
		}
		else {
			if (CheckBarricade(rPos, true)) {
				while (1) {
					random_pos = random(dre);

					same = false;
					for (auto r : v)
					{
						if (r == random_pos) {
							same = true;
							break;
						}
					}

					if (same) {
						continue;
					}

					rPos.x = barricade_pos[random_pos].x + pos.x;
					rPos.z = barricade_pos[random_pos].z + pos.z;

					if (CheckBarricade(rPos, false))
						continue;

					break;
				}
			}
			ChangeWall(rPos, true);
			base[t].x = rPos.x;
			base[t].z = rPos.z;
			base[t].dir = DIR::WIDTH;
			t++;
		}
	}
}

void Map::InputDoor(float row, float col, float size_x, float size_z, DIR dir, int num)
{
	Object obj;
	
	obj.col = (int)col;
	obj.row = (int)row;
	obj.size_x = size_x;
	obj.size_z = size_z;
	obj.state = ObjectState::NORMAL;
	obj.type = ObjectType::DOOR;
	obj.x = row;
	obj.z = col;
	obj.dir = dir;
	obj.num = num;

	map_info.Door.emplace_back(obj);
}

void Map::MakeDoor()
{
	InputDoor(41, 85, 7, 1, DIR::WIDTH, 0);
	InputDoor(211, 238, 11, 1, DIR::WIDTH, 1);
	InputDoor(320, 334, 1, 9, DIR::HEIGHT, 2);
	InputDoor(509, 334, 1, 11, DIR::HEIGHT, 3);
	InputDoor(630, 334, 1, 11, DIR::HEIGHT, 4);
	InputDoor(818, 334, 1, 15, DIR::HEIGHT, 5);
}

void Map::EditMap(iPos sp, iPos ep)
{
	for (int i = sp.z + 1; i < ep.z; ++i)
	{
		for (int j = sp.x + 1; j < ep.z; ++j)
		{
			if (map[i][j] == (char)MazeWall::BARRICADE)
			{
				if (map[i][j + 1] == (char)MazeWall::ROAD && map[i + 1][j] == (char)MazeWall::ROAD && map[i + 1][j + 1] == (char)MazeWall::BARRICADE)
				{
					map[i][j + 1] = (char)MazeWall::BARRICADE;
				}
				if (map[i][j - 1] == (char)MazeWall::ROAD && map[i + 1][j] == (char)MazeWall::ROAD && map[i - 1][j + 1] == (char)MazeWall::BARRICADE)
				{
					map[i][j - 1] = (char)MazeWall::BARRICADE;
				}
			}
		}
	}
}

void Map::MazeTypeInit(iPos* base, BarricadeType b_type, int num)
{
	for (int i = 0; i < num; ++i)
	{
		base[i].b_type = b_type;
	}
}

void Map::Initialize()
{
	MakeDoor();
	MakeMaze(One_Base_Pos, One_Level, one_base, One_Base_End_Pos);
	MakeMaze(TWO_Base_Pos, Two_Level, two_base, TWO_Base_End_Pos);
	MakeMaze(THREE_Base_Pos, Three_Level, three_base, THREE_Base_End_Pos);
	MakeMaze(THREE_Base_Pos2, Three_Level, three_base2, THREE_Base_End_Pos2);

	MazeTypeInit(one_base, BarricadeType::CAR, One_Level);
	MazeTypeInit(two_base, BarricadeType::TREE, Two_Level);
	MazeTypeInit(three_base, BarricadeType::BARRICADE, Three_Level);
	MazeTypeInit(three_base2, BarricadeType::BARRICADE, Three_Level);

	EditMap(One_Base_Pos, One_Base_End_Pos);
	EditMap(TWO_Base_Pos, TWO_Base_End_Pos);
	EditMap(THREE_Base_Pos, THREE_Base_End_Pos);
	EditMap(THREE_Base_Pos2, THREE_Base_End_Pos2);
}

AS_Map::AS_Map()
{
	ZeroMemory(&as_map, sizeof(as_map));
}

AS_Map::~AS_Map()
{

}

void AS_Map::InitMap(Map& map)
{
	for (int i = 0; i < WORLD_HEIGHT; ++i)
	{
		for (int j = 0; j < WORLD_WIDTH; ++j)
		{
			if (map.map[i][j] == (char)MazeWall::ROAD)
				as_map[i][j] = AS_MAP_TYPE::ROAD;
			else
				as_map[i][j] = AS_MAP_TYPE::WALL;
		}
	}
}