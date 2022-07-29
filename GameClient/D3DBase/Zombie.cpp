#include "Zombie.h"
#include "GameFramework.h"

#define MAX_QUEUE_SIZE 3

Zombie::Zombie()
{
	view = false;
	dir = Direction::NONE;
	arrive = false;
}

Zombie::~Zombie()
{

}

void Zombie::SetInfo(sc_zombie_move_packet* packet)
{
	//if (abs(arr[id].x - packet->x) + abs(arr[id].z - packet->z) > 0.2f) {
	//arr[id].x = packet->x;
	//arr[id].z = packet->z;
	//	cout << "ÁÂÇ¥ ¹Ù²ñ" << endl;
	//}

	speed = packet->speed;
	t_x = packet->t_x;
	t_z = packet->t_z;
	dir = packet->dir;
	arrive = false;

	if (packet->x < 50 || packet->x > 1100 || packet->z < 50 || packet->z > 420)
		return;

	AddMove(packet->x, packet->z);
}

bool Zombie::IsCollied(int r, int c, char map[WORLD_HEIGHT][WORLD_WIDTH])
{
	if (map[r][c] != (char)MazeWall::ROAD)
		return true;

	return false;
}

void Zombie::AddMove(float x, float z)
{
	MoveInfo info;
	info.x = x;
	info.z = z;
	info.time = CGameFramework::GetInstance()->GetTotalTime();
	move_list.push_back(info);
	/*while (move_queue.size() > MAX_QUEUE_SIZE) {
		move_queue.pop_front();
	}*/
}

void Zombie::Move()
{
	float time = CGameFramework::GetInstance()->GetTotalTime() - 0.5f;
	if (move_list.size() < 2) return;
	/*while ((*move_queue.begin()).time < time) {
		move_queue.pop_front();
	}*/

	const auto& first = *(move_list.begin());
	const auto& second = *(++move_list.begin());
	if (second.time < time) {
		move_list.pop_front();
		Move();
		return;
	}
	LinearMove(first, second, time);
}

void Zombie::LinearMove(const MoveInfo& a, const MoveInfo& b, float time)
{
	float t = b.time - a.time;
	float factor = (time - a.time) / t;

	x = a.x * (1 - factor) + b.x * factor;
	z = a.z * (1 - factor) + b.z * factor;
}