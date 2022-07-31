#include "clientinfo.h"

Client::Client() 
{
	_type = PlayerType::NONE;
	send_start_packet = false;
	mouse_click_time = chrono::system_clock::now();
	idle_time = chrono::system_clock::now();
	server_time = 1000;
	map_type = MapType::SPAWN;
	_zombie_send_overflow = false;
}

Client::~Client()
{
	delete player;
	player = nullptr;
}

void Client::do_recv()
{
	m_socket.do_recv(prev_size);
}

void Client::do_send(int num_bytes, void* mess)
{
	m_socket.do_send(num_bytes, mess);
}

Player::Player()
{
	dir = Direction::NONE;
}

Player::~Player()
{

}

bool Player::IsCollied(int r, int c, Map& map)
{
	if (map.map[r][c] != (char)MazeWall::ROAD)
		return true;

	return false;
}
/*
bool Player::Move(Direction dir,  Map& map)
{
	bool collied = false;

	float m_speed = speed;

	switch (dir)
	{
	case Direction::UP:
	{
		int row = (int)(z + m_speed);
		int col = (int)x;

		float row_result = (z + m_speed) - row;
		float col_result = x - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		for (int i = (int)z; i <= row; ++i)
		{
			collied = IsCollied(i, col, map);
			if (collied)
			{
				z = float(i - 1);
				break;
			}
		}

		if (collied == false)
			z += m_speed;

		return true;
		break;
	}
	case Direction::DOWN:
	{
		int row = (int)(z - m_speed);
		int col = (int)x;

		float row_result = (z - m_speed) - row;
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

		if(collied == false)
			z -= m_speed;

		return true;
		break;
	}
	case Direction::LEFT:
	{
		int row = (int)z;
		int col = (int)(x - m_speed);

		float row_result = z - row;
		float col_result = (x - m_speed) - col;
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

		if(collied == false)
			x -= m_speed;

		return true;
		break;
	}
	case Direction::RIGHT:
	{
		int row = (int)z;
		int col = (int)(x + m_speed);

		float row_result = z - row;
		float col_result = (x + m_speed) - col;
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

		if(collied == false)
			x += m_speed;

		return true;
		break;
	}
	case Direction::UP_LEFT:
	{
		int row = (int)(z + (m_speed * 0.7f));
		int col = (int)(x - (m_speed * 0.7f));

		float row_result = (z + (m_speed * 0.7f)) - row;
		float col_result = (x - (m_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		int row2 = (int)z;
		int col2 = (int)x;

		float row_result2 = z - row2;
		float col_result2 = x - col2;
		if (row_result2 >= 0.5f)
			row2 += 1;
		if (col_result2 >= 0.5f)
			col2 += 1;

		int x_num = 2;
		int line = 0;
		for (int i = row2; i <= row; ++i)
		{
			if (i == row2)
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

			if (x_num == 2 && i == row2)
			{
				for (int j = col2; j >= col2 - 1; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == col2) && (i == row2))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == col2 - 1) && (i == row2))
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
				for (int j = col2 - line + 1; j >= col2 - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (col2 - line + 1 == j)
						{
							x = (float)j;
							z = (float)(i - 1);
						}
						else if (col2 - line == j)
						{
							x =(float)(j + 1);
							z =(float)(i - 1);
						}
						break;
					}
				}
			}
			else if (x_num == 3)
			{
				for (int j = col2 - line + 1; j >= col2 - 1 - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((col2 - line + 1 == j) && (i == row2 + line))
						{
							x = (float)j;
							z = (float)(i - 1);
							break;
						}
						else if ((col2 - line == j) && (i == row2 + line))
						{
							x = (float)(j + 1);
							z = (float)(i - 1);
							break;
						}
						else if ((col2 - line - 1 == j) && (i == row2 + line))
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
			x -= m_speed * 0.7f;
			z += m_speed * 0.7f;
		}
		return true;
		break;
	}
	case Direction::UP_RIGHT:
	{
		int row = (int)(z + (m_speed * 0.7f));
		int col = (int)(x + (m_speed * 0.7f));

		float row_result = (z + (m_speed * 0.7f)) - row;
		float col_result = (x + (m_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		int row2 = (int)z;
		int col2 = (int)x;

		float row_result2 = z - row2;
		float col_result2 = x  - col2;
		if (row_result2 >= 0.5f)
			row2 += 1;
		if (col_result2 >= 0.5f)
			col2 += 1;

		int x_num = 2;
		int line = 0;
		for (int i = row2; i <= row; ++i)
		{
			if (i == row2)
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

			if (x_num == 2 && i == row2)
			{
				for (int j = col2 ; j <= col2 + 1; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == col2) && (i == row2))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == col2 + 1) && (i == row2))
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
				for (int j = col2 + line - 1; j <= col2 + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (col2 + line - 1 == j)
						{
							x = (float)j;
							z = (float)(i - 1);
						}
						else if (col2 + line == j)
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
					for (int j = col2 + line - 1; j <= col2 + 1 + line; ++j)
					{
						collied = IsCollied(i, j, map);
						if (collied)
						{
							if ((col2 + line - 1 == j) && (i == row2 + line))
							{
								x = (float)j;
								z = (float)(i - 1);
								break;
							}
							else if ((col2 + line == j) && (i == row2 + line))
							{
								x = (float)(j - 1);
								z = (float)(i - 1);
								break;
							}
							else if ((col2 + line + 1 == j) && (i == row2 + line))
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
			x += m_speed * 0.7f;
			z += m_speed * 0.7f;
		}
		return true;
		break;
	}
	case Direction::DOWN_LEFT:
	{
		int row = (int)(z - (m_speed * 0.7f));
		int col = (int)(x - (m_speed * 0.7f));

		float row_result = (z - (m_speed * 0.7f)) - row;
		float col_result = (x - (m_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		int row2 = (int)z;
		int col2 = (int)x;

		float row_result2 = z - row2;
		float col_result2 = x - col2;
		if (row_result2 >= 0.5f)
			row2 += 1;
		if (col_result2 >= 0.5f)
			col2 += 1;

		int x_num = 2;
		int line = 0;
		for (int i = row2; i >= row; --i)
		{
			if (i == row2)
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

			if (x_num == 2 && i == row2)
			{
				for (int j = col2; j >= col2 - 1; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == col2) && (i == row2))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == col2 - 1) && (i == row2))
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
				for (int j = col2 - line+1; j >= col2 - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (col2 - line+1 == j)
						{
							x = (float)j;
							z = (float)(i + 1);
						}
						else if (col2 - line == j)
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
				for (int j = col2 - line + 1; j >= col2 - 1 - line; --j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((col2 - line + 1 == j) && (i == row2 - line))
						{
							x = (float)j;
							z = (float)(i + 1);
							break;
						}
						else if ((col2 - line == j) && (i == row2 - line))
						{
							x = (float)(j + 1);
							z = (float)(i + 1);
							break;
						}
						else if ((col2 - line - 1 == j) && (i == row2 - line))
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
			x -= m_speed * 0.7f;
			z -= m_speed * 0.7f;
		}

		return true;
		break;
	}
	case Direction::DOWN_RIGHT:
	{
		int row = (int)(z - (m_speed * 0.7f));
		int col = (int)(x + (m_speed * 0.7f));

		float row_result = (z - (m_speed * 0.7f)) - row;
		float col_result = (x + (m_speed * 0.7f)) - col;
		if (row_result >= 0.5f)
			row += 1;
		if (col_result >= 0.5f)
			col += 1;

		int row2 = (int)z;
		int col2 = (int)x;

		float row_result2 = z - row2;
		float col_result2 = x - col2;
		if (row_result2 >= 0.5f)
			row2 += 1;
		if (col_result2 >= 0.5f)
			col2 += 1;

		int x_num = 2;
		int line = 0;
		for (int i = row2; i >= row; --i)
		{
			if (i == row2)
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

			if (x_num == 2 && i == row2)
			{
				for (int j = col2; j <= col2 + 1; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((j == col2) && (i == row2))
						{
							cout << "이럴 수 없다 \n";
						}
						else if ((j == col2 + 1) && (i == row2))
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
				for (int j = col2 + line -1; j <= col2 + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if (col2 + line - 1 == j)
						{
							x = (float)j;
							z = (float)(i + 1);
						}
						else if (col2 + line == j)
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
				for (int j = col2 + line - 1; j <= col2 + 1 + line; ++j)
				{
					collied = IsCollied(i, j, map);
					if (collied)
					{
						if ((col2 + line - 1 == j) && (i == row2 - line))
						{
							x = (float)j;
							z = (float)(i + 1);
							break;
						}
						else if ((col2 + line == j) && (i == row2 - line))
						{
							x = (float)(j - 1);
							z = (float)(i + 1);
							break;
						}
						else if ((col2 + line + 1 == j) && (i == row2 - line))
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
			x += m_speed * 0.7f;
			z -= m_speed * 0.7f;
		}

		return true;
		break;
	}
	default:
	{
		cout << "잘못된 이동입니다 \n";
		return false;
	}
	}

	return false;
}
*/

bool Player::PlayerAttack(float x1, float z1, float z_x, float z_z)
{
	float degree = (z1 / x1) * (-1.0f);
	
	float a_result = abs((degree * z_x) + (-1.0f * z_z) - (degree * x) + z) / sqrt(degree * degree + 1);

	if (-1.0f < a_result && a_result < 1.0f)	// 절대적인 거리가 1.0f안이면 충돌한것으로 판정하여 true를 반환, 그 외는 아니므로 false
	{
		return true;
	}
	else
		return false;
}

Commander::Commander()
{
	//hp = 100, maxhp = 100;
	hp = 1000, maxhp = 1000;
	attack = 5;
	interaction = 10;
	x = 63.0f , z = 55.0f;
	//x = 202.0f, z = 244.0f;
	//speed = 1.0f;
	speed = 5.0f;
	//bullet = 30;
	special_skill = 1;
	kill_zombie = 0;
	special_check = false;
	special_time = chrono::system_clock::now();
}

Commander::~Commander()
{
	
}

Engineer::Engineer()
{
	//hp = 75, maxhp = 75;
	hp = 100, maxhp = 100;
	attack = 3;
	interaction = 15;
	x = 65.0f, z = 55.0f;
	//x = 203.0f, z = 244.0f;
	//speed = 0.8f;
	speed = 4.0f;
	//bullet = 30;
	special_skill = 3;
	special_dir = DIR::HEIGHT;
	kill_zombie = 0;
	special_check = false;
	special_time = chrono::system_clock::now();
}

Engineer::~Engineer()
{

}

Mercynary::Mercynary()
{
	//hp = 125, maxhp = 125;
	hp = 1000, maxhp = 1000;
	attack = 7;
	interaction = 5;
	x = 67.0f, z = 55.0f;
	//x = 204.0f, z = 244.0f;
	//speed = 1.2f;
	speed = 6.0f;
	//bullet = 30;
	special_skill = 3;
	kill_zombie = 0;
	special_check = false;
	special_time = chrono::system_clock::now();
}

Mercynary::~Mercynary()
{

}