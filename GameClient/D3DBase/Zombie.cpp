#include "Zombie.h"

Zombie::Zombie()
{
	view = false;
	dir = Direction::NONE;
	arrive = false;
}

Zombie::~Zombie()
{

}

bool Zombie::IsCollied(int r, int c, char map[WORLD_HEIGHT][WORLD_WIDTH])
{
	if (map[r][c] != (char)MazeWall::ROAD)
		return true;

	return false;
}