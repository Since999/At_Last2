#include "zombieinfo.h"

Zombie::Zombie()
{
	zombie_dir = Direction::NONE;
}

Zombie::~Zombie()
{
}

Direction Zombie::RootDir(float s_x, float s_z, float e_x, float e_z)
{
	float dir_x = e_x - s_x;
	float dir_z = e_z - s_z;

	float radian = atan2f(dir_z, dir_x);
	float degree = radian * (180 / 3.141592f);

	if (degree < 0)
		degree += 360.0f;

	if (67.5f <= degree && degree < 112.5f)
	{
		return Direction::UP;
	}
	else if (112.5f <= degree && degree < 157.5f)
	{
		return Direction::UP_LEFT;
	}
	else if (157.5f <= degree && degree < 202.5f)
	{
		return Direction::LEFT;
	}
	else if (202.5f <= degree && degree < 247.5f)
	{
		return Direction::DOWN_LEFT;
	}
	else if (247.5f <= degree && degree < 292.5f)
	{
		return Direction::DOWN;
	}
	else if (292.5f <= degree && degree < 337.5f)
	{
		return Direction::DOWN_RIGHT;
	}
	else if (337.5f <= degree && degree < 360.0f || 0 <= degree && degree < 22.5f)
	{
		return Direction::RIGHT;
	}
	else if (22.5f <= degree && degree < 67.5f)
	{
		return Direction::UP_RIGHT;
	}

	cout << "여기로 오면 안돼... \n";
	return Direction::NONE;
}

bool Zombie::IsCollied(int r, int c, Map& map)
{
	if (map.map[r][c] != (char)MazeWall::ROAD)
		return true;

	return false;
}

float Zombie::Distance(float s_x, float s_z, float e_x, float e_z)
{
	float distance = (float)sqrt(((e_x - s_x) * (e_x - s_x)) + ((e_z - s_z) * (e_z - s_z)));

	return distance;
}

MoveResult Zombie::ZombieMove(float z_speed, Map& map)
{
	if (root.empty())
	{
		return MoveResult::FAIL;
	}

	AS_Node* node = root.top();

	float distance = Distance(x, z, node->Get_X(), node->Get_Y());

	float dir_x = node->Get_X() - x;
	float dir_z = node->Get_Y() - z;

	float radian = atan2f(dir_z, dir_x);
	float degree = radian * (180 / 3.141592f);

	if (degree < 0)
		degree += 360.0f;

	if (67.5f <= degree && degree < 112.5f)
	{
		zombie_dir = Direction::UP;
	}
	else if (112.5f <= degree && degree < 157.5f)
	{
		zombie_dir = Direction::UP_LEFT;
	}
	else if (157.5f <= degree && degree < 202.5f)
	{
		zombie_dir = Direction::LEFT;
	}
	else if (202.5f <= degree && degree < 247.5f)
	{
		zombie_dir = Direction::DOWN_LEFT;
	}
	else if (247.5f <= degree && degree < 292.5f)
	{
		zombie_dir = Direction::DOWN;
	}
	else if (292.5f <= degree && degree < 337.5f)
	{
		zombie_dir = Direction::DOWN_RIGHT;
	}
	else if (337.5f <= degree && degree < 360.0f || 0 <= degree && degree < 22.5f)
	{
		zombie_dir = Direction::RIGHT;
	}
	else if (22.5f <= degree && degree < 67.5f)
	{
		zombie_dir = Direction::UP_RIGHT;
	}

	if (distance > z_speed)
	{
		float fix_speed = z_speed;

		float cos_angle = cos(degree * (3.141592f / 180.0f));
		float sin_angle = sin(degree * (3.141592f / 180.0f));

		float t_x = cos_angle * fix_speed + x;
		float t_z = sin_angle * fix_speed + z;

		x = t_x;
		z = t_z;

		return MoveResult::MOVE;
	}
	else
	{
		float fix_speed = distance;

		float cos_angle = cos(degree * (3.141592f / 180.0f));
		float sin_angle = sin(degree * (3.141592f / 180.0f));

		float t_x = cos_angle * fix_speed + x;
		float t_z = sin_angle * fix_speed + z;

		root.pop();

		node = root.top();

		fix_speed = z_speed - distance;

		dir_x = node->Get_X() - x;
		dir_z = node->Get_Y() - z;

		radian = atan2f(dir_z, dir_x);
		degree = radian * (180 / 3.141592f);

		if (degree < 0)
			degree += 360.0f;

		cos_angle = cos(degree * (3.141592f / 180.0f));
		sin_angle = sin(degree * (3.141592f / 180.0f));

		t_x = cos_angle * fix_speed + x;
		t_z = sin_angle * fix_speed + z;

		x = t_x;
		z = t_z;

		return MoveResult::MOVE;
	}
}

MoveResult Zombie::Move(float z_speed, Map& map)
{
	if (root.empty())
	{
		return MoveResult::FAIL;
	}

	AS_Node* node = root.top();

	// root_x, root_z 는 이동하기 전의 기존 위치
	int root_x = (int)x;
	int root_z = (int)z;
	
	float result_root_x = x - root_x;
	float result_root_z = z - root_z;

	if (result_root_x > 0.5f)
		root_x += 1;
	if (result_root_z > 0.5f)
		root_z += 1;

	// 이동하는 방향이 다르면 DIR 결정 아니면 root를 pop하고 반환
	if (node->Get_X() != root_x || node->Get_Y() != root_z)
	{
		zombie_dir = RootDir((float)root_x, (float)root_z, (float)node->Get_X(), (float)node->Get_Y());
	}
	else
	{
		//cout << root.top()->Get_X() << ", " << root.top()->Get_Y() << "에서";
		root.pop();
		//cout << root.top()->Get_X() << ", " << root.top()->Get_Y() << "로 변경되었다. \n";
		return MoveResult::MOVE;
	}

	// node에 아무것도 없다면? 움직일 수 없으므로 반환
	if (node == nullptr)
	{
		return MoveResult::FAIL;
	}

	// node가 맵 밖을 가르킨다면..? 당연히 오류이므로 제거
	if (node->Get_X() < 0 || node->Get_X() > 1100 || node->Get_Y() < 0 || node->Get_Y() > 420)
	{
		while (!root.empty())
			root.pop();
		return MoveResult::FAIL;
	}

	// 좀비 방향이 NONE은 해당 좌표에 도착한 것이므로 ARRIVE 반환
	if (zombie_dir == Direction::NONE)
	{
		return MoveResult::ARRIVE;
	}

	bool collied = false;

	// 좀비의 방향에 따라 이동을 한다.
	switch (zombie_dir)
	{
	case Direction::UP:
	{
		int row = (int)(z + z_speed);
		int col = (int)x;

		float row_result = (z + z_speed) - row;
		float col_result = x - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		for (int i = root_z; i <= row; ++i)
		{
			collied = IsCollied(i, col, map);
			if (collied)
			{
				if (i == root_z)
					z = root_z;
				else
					z = i - 1;
				break;
			}
		}

		if (collied == false)
		{
			z += z_speed;
		}
		else
		{
			cout << "UP 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	case Direction::UP_RIGHT:
	{
		int row = (int)(z + (z_speed * 0.7f));
		int col = (int)(x + (z_speed * 0.7f));

		float row_result = (z + (z_speed * 0.7f)) - row;
		float col_result = (x + (z_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		if (true == IsCollied(root_z + 1, root_x, map))
		{
			//cout << "UP_RIGHT 중 UP 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x + 1);
			add_node.Set_Y(root_z);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();

			return MoveResult::MOVE;
		}
		if (true == IsCollied(root_z, root_x + 1, map))
		{
			//cout << "UP_RIGHT 중 RIGHT 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x);
			add_node.Set_Y(root_z + 1);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();

			return MoveResult::MOVE;
		}

		int x_num = 2;
		int line = 0;
		for (int i = root_z; i <= row; ++i)
		{
			if (i == root_z)
				x_num = 2;
			else if (i == row)
			{
				x_num = 2;
				line++;
			}
			else
			{
				x_num = 3;
				line++;
			}

			if (x_num == 2 && i == root_x)
			{
				for (int j = root_x; j <= root_x + 1; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == root_x) && (i == root_z))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == root_x + 1) && (i == root_z))
						{
							x = (float)(j - 1);
							z = (float)i;
						}
						break;
					}
				}
			}
			else if (x_num == 2 && i == row)
			{
				for (int j = root_x + line - 1; j <= root_x + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (root_x + line - 1 == j)
						{
							x = (float)j;
							z = (float)(i - 1);
						}
						else if (root_x + line == j)
						{
							x = (float)(j - 1);
							z = (float)(i - 1);
						}
						break;
					}
				}
			}
			else if (x_num == 3)
			{
				for (int j = root_x + line - 1; j <= root_x + 1 + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((root_x + line - 1 == j) && (i == root_z + line))
						{
							x = (float)j;
							z = (float)(i - 1);
							break;
						}
						else if ((root_x + line == j) && (i == root_z + line))
						{
							x = (float)(j - 1);
							z = (float)(i - 1);
							break;
						}
						else if ((root_x + line + 1 == j) && (i == root_z + line))
						{
							x = (float)(j - 1);
							z = (float)i;
							break;
						}
						break;
					}
				}
			}

			if (collied)
				break;
		}

		if (collied == false)
		{
			x += z_speed * 0.7f;
			z += z_speed * 0.7f;
		}
		else
		{
			x -= z_speed * 0.7f;
			z -= z_speed * 0.7f;
			cout << "UP_RIGHT 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	case Direction::UP_LEFT:
	{
		int row = (int)(z + (z_speed * 0.7f));
		int col = (int)(x - (z_speed * 0.7f));

		float row_result = (z + (z_speed * 0.7f)) - row;
		float col_result = (x - (z_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		//if (col_result >= 0.5f && row_result >= 0.5f)
		//	collied = IsCollied(row - 1, col - 1, map);
		//if (col_result >= 0.5f && row_result < 0.5f)
		//	collied = IsCollied(row, col - 1, map);
		//else if (col_result < 0.5f && row_result >= 0.5f)
		//	collied = IsCollied(row - 1, col, map);
		//else

		if (true == IsCollied(root_z + 1, root_x, map))
		{
			//cout << "UP_LEFT 중 UP 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x - 1);
			add_node.Set_Y(root_z);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();
			return MoveResult::MOVE;
		}
		if (true == IsCollied(root_z, root_x - 1, map))
		{
			//cout << "UP_LEFT 중 LEFT 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x);
			add_node.Set_Y(root_z + 1);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();
			return MoveResult::MOVE;
		}

		int x_num = 2;
		int line = 0;
		for (int i = root_z; i <= row; ++i)
		{
			if (i == root_z)
				x_num = 2;
			else if (i == row)
			{
				x_num = 2;
				line++;
			}
			else
			{
				x_num = 3;
				line++;
			}

			if (x_num == 2 && i == root_z)
			{
				for (int j = root_x; j >= root_x - 1; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == root_x) && (i == root_z))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == root_x - 1) && (i == root_z))
						{
							x = (float)(j + 1);
							z = (float)i;
						}
						break;
					}
				}
			}
			else if (x_num == 2 && i == row)
			{
				for (int j = root_x - line + 1; j >= root_x - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (root_x - line + 1 == j)
						{
							x = (float)j;
							z = (float)(i - 1);
						}
						else if (root_x - line == j)
						{
							x = (float)(j + 1);
							z = (float)(i - 1);
						}
						break;
					}
				}
			}
			else if (x_num == 3)
			{
				for (int j = root_x - line + 1; j >= root_x - 1 - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((root_x - line + 1 == j) && (i == root_z + line))
						{
							x = (float)j;
							z = (float)(i - 1);
							break;
						}
						else if ((root_x - line == j) && (i == root_z + line))
						{
							x = (float)(j + 1);
							z = (float)(i - 1);
							break;
						}
						else if ((root_x - line - 1 == j) && (i == root_z + line))
						{
							x = (float)(j + 1);
							z = (float)i;
							break;
						}
						break;
					}
				}
			}

			if (collied)
				break;
		}

		if (collied == false)
		{
			x -= z_speed * 0.7f;
			z += z_speed * 0.7f;
		}
		else
		{
			x += z_speed * 0.7f;
			z -= z_speed * 0.7f;
			cout << "UP_LEFT 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	case Direction::LEFT:
	{
		int row = (int)z;
		int col = (int)(x - z_speed);

		float row_result = z - row;
		float col_result = (x - z_speed) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		for (int i = (int)x; i >= col; --i)
		{
			collied = IsCollied(row, i, map);
			if (collied)
			{
				x = (float)(i + 1);
				break;
			}
		}

		if (collied == false)
		{
			x -= z_speed;
		}
		else
		{
			x += z_speed;
			cout << "LEFT 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;

		break;
	}
	case Direction::RIGHT:
	{
		int row = (int)z;
		int col = (int)(x + z_speed);

		float row_result = z - row;
		float col_result = (x + z_speed) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		for (int i = (int)x; i <= col; ++i)
		{
			collied = IsCollied(row, i, map);
			if (collied)
			{
				x = (float)(i - 1);
				break;
			}
		}

		if (collied == false)
		{
			x += z_speed;
		}
		else
		{
			x -= z_speed;
			cout << "RIGHT 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	case Direction::DOWN:
	{
		int row = (int)(z - z_speed);
		int col = (int)x;

		float row_result = (z - z_speed) - row;
		float col_result = x - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		for (int i = (int)z; i >= row; --i)
		{
			collied = IsCollied(i, col, map);
			if (collied)
			{
				z = float(i + 1);
				break;
			}
		}

		if (collied == false)
		{
			z -= z_speed;
		}
		else
		{
			z += z_speed;
			cout << "DOWN 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	case Direction::DOWN_LEFT:
	{
		int row = (int)(z - (z_speed * 0.7f));
		int col = (int)(x - (z_speed * 0.7f));

		float row_result = (z - (z_speed * 0.7f)) - row;
		float col_result = (x - (z_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		if (true == IsCollied(root_z - 1, root_x, map))
		{
			//cout << "DOWN_LEFT 중 DOWN 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x - 1);
			add_node.Set_Y(root_z);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();
			return MoveResult::MOVE;
		}
		if (true == IsCollied(root_z, root_x - 1, map))
		{
			//cout << "DOWN_LEFT 중 LEFT 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x);
			add_node.Set_Y(root_z - 1);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();
			return MoveResult::MOVE;
		}

		int x_num = 2;
		int line = 0;
		for (int i = root_z; i >= row; --i)
		{
			if (i == root_z)
				x_num = 2;
			else if (i == row)
			{
				x_num = 2;
				line++;
			}
			else
			{
				x_num = 3;
				line++;
			}

			if (x_num == 2 && i == root_z)
			{
				for (int j = root_x; j >= root_x - 1; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == root_x) && (i == root_z))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == root_x - 1) && (i == root_z))
						{
							x = (float)(j + 1);
							z = (float)i;
						}
						break;
					}
				}
			}
			else if (x_num == 2 && i == row)
			{
				for (int j = root_x - line + 1; j >= root_x - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (root_x - line + 1 == j)
						{
							x = (float)j;
							z = (float)(i + 1);
						}
						else if (root_x - line == j)
						{
							x = (float)(j + 1);
							z = (float)(i + 1);
						}
						break;
					}
				}
			}
			else if (x_num == 3)
			{
				for (int j = root_x - line + 1; j >= root_x - 1 - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((root_x - line + 1 == j) && (i == root_z - line))
						{
							x = (float)j;
							z = (float)(i + 1);
							break;
						}
						else if ((root_x - line == j) && (i == root_z - line))
						{
							x = (float)(j + 1);
							z = (float)(i + 1);
							break;
						}
						else if ((root_x - line - 1 == j) && (i == root_z - line))
						{
							x = (float)(j + 1);
							z = (float)i;
							break;
						}
						break;
					}
				}
			}

			if (collied)
				break;
		}

		if (collied == false)
		{
			x -= z_speed * 0.7f;
			z -= z_speed * 0.7f;
		}
		else
		{
			x += z_speed * 0.7f;
			z += z_speed * 0.7f;
			cout << "DOWN_LEFT 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	case Direction::DOWN_RIGHT:
	{
		int row = (int)(z - (z_speed * 0.7f));
		int col = (int)(x + (z_speed * 0.7f));

		float row_result = (z - (z_speed * 0.7f)) - row;
		float col_result = (x + (z_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		if (true == IsCollied(root_z - 1, root_x, map))
		{
			//cout << "DOWN_RIGHT 중 DOWN 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x + 1);
			add_node.Set_Y(root_z);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();
			return MoveResult::MOVE;
		}
		if (true == IsCollied(root_z, root_x + 1, map))
		{
			//cout << "DOWN_RIGHT 중 RIGHT 이 막힘 \n";
			//cout << "root_x : " << root_x << ", root_z : " << root_z << "\n";

			int temp_x = node->Get_X();
			int temp_z = node->Get_Y();

			z_move_lock.lock();
			add_node = *root.top();
			z_move_lock.unlock();

			add_node.Set_X(root_x);
			add_node.Set_Y(root_z - 1);

			z_move_lock.lock();
			root.push(&add_node);
			root.push(&add_node);
			z_move_lock.unlock();

			node = root.top();
			return MoveResult::MOVE;
		}

		int x_num = 2;
		int line = 0;
		for (int i = root_z; i >= row; --i)
		{
			if (i == root_z)
				x_num = 2;
			else if (i == row)
			{
				x_num = 2;
				line++;
			}
			else
			{
				x_num = 3;
				line++;
			}

			if (x_num == 2 && i == root_z)
			{
				for (int j = root_x; j <= root_x + 1; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == root_x) && (i == root_z))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == root_x + 1) && (i == root_z))
						{
							x = (float)(j - 1);
							z = (float)i;
						}
						break;
					}
				}
			}
			else if (x_num == 2 && i == row)
			{
				for (int j = root_x + line - 1; j <= root_x + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (root_x + line - 1 == j)
						{
							x = (float)j;
							z = (float)(i + 1);
						}
						else if (root_x + line == j)
						{
							x = (float)(j - 1);
							z = (float)(i + 1);
						}
						break;
					}
				}
			}
			else if (x_num == 3)
			{
				for (int j = root_x + line - 1; j <= root_x + 1 + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((root_x + line - 1 == j) && (i == root_z - line))
						{
							x = (float)j;
							z = (float)(i + 1);
							break;
						}
						else if ((root_x + line == j) && (i == root_z - line))
						{
							x = (float)(j - 1);
							z = (float)(i + 1);
							break;
						}
						else if ((root_x + line + 1 == j) && (i == root_z - line))
						{
							x = (float)(j - 1);
							z = (float)i;
							break;
						}
						break;
					}
				}
			}

			if (collied)
				break;
		}

		if (collied == false)
		{
			x += z_speed * 0.7f;
			z -= z_speed * 0.7f;
		}
		else
		{
			x -= z_speed * 0.7f;
			z += z_speed * 0.7f;
			cout << "DOWN_RIGHT 충돌 \n";
			return MoveResult::COLLIED;
		}

		return MoveResult::MOVE;
		break;
	}
	}

	return MoveResult::FAIL;
}

NPC::NPC()
{
	attack_delay_time = chrono::system_clock::now();
}

NPC::~NPC()
{
	//if (zombie != nullptr) {
	//	delete zombie;
	//	zombie = nullptr;
	//}
}

NormalZombie::NormalZombie()
{
	hp = 30;
	damage = 5;
	attRange = 2.5f;
	speed = 5.0f;
	infection = 1.0f;
	_type = ZombieType::NORMAL;
	attack_timer = chrono::milliseconds(2000);
	searchRange = 10.0f;
	angle = 0.0f;
}

NormalZombie::~NormalZombie()
{
}

SoldierZombie::SoldierZombie()
{
	hp = 40;
	damage = 6;
	attRange = 2.8f;
	speed = 5.5f;
	infection = 3.0f;
	_type = ZombieType::SOLIDEIR;
	attack_timer = chrono::milliseconds(2000);
	searchRange = 10.0f;
	angle = 0.0f;
}

SoldierZombie::~SoldierZombie()
{
}

TankerZombie::TankerZombie()
{
	hp = 50;
	damage = 8;
	attRange = 2.2f;
	speed = 4.5f;
	infection = 2.0f;
	_type = ZombieType::TANKER;
	attack_timer = chrono::milliseconds(2000);
	searchRange = 10.0f;
	angle = 0.0f;
}

TankerZombie::~TankerZombie()
{
}

DogZombie::DogZombie()
{
	hp = 20;
	damage = 4;
	attRange = 2.0f;
	speed = 7.2f;
	infection = 2.0f;
	_type = ZombieType::DOG;
	attack_timer = chrono::milliseconds(2000);
	searchRange = 10.0f;
	angle = 0.0f;
}

DogZombie::~DogZombie()
{
}