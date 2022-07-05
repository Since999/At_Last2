#include "stdafx.h"
#include "Client.h"

Client::Client()
{
	_type = PlayerType::NONE;
	move_time = chrono::system_clock::now();
	t_x = 1;
	t_z = 0;
	_dir = Direction::NONE;
	rotate_speed = MathHelper::Pi *2;
	special_skill_key = false;
	special_id = -1;
}

Client::~Client()
{

}

bool Client::IsCollied(int r, int c, char map[WORLD_HEIGHT][WORLD_WIDTH])
{
	if (map[r][c] != (char)MazeWall::ROAD)
		return true;

	return false;
}
#include "Network.h"

void Client::move(float time_elapsed)
{
	/*if (abs(t_x) < 0.1f && abs(t_z) < 0.1f) {
		speed = clamp( speed - acceleration * time_elapsed, 0.0f, max_speed);
		return;
	}*/
	speed = clamp(speed - (acceleration / 2) * time_elapsed, 0.0f, max_speed);
	if(is_input) speed = clamp(speed + acceleration * time_elapsed, 0.0f, max_speed);
	
	if(is_input){
		//rotation
		float real_angle = 0.0f;
		real_angle = rotate_speed * time_elapsed;
		if (rotation_angle < 0.0f) {
			real_angle = -real_angle;
		}
		angle += real_angle;
		
		t_x = cos(angle);
		t_z = sin(angle);
		
		rotation_angle = 0.0f;
	}
	float tmp_x, tmp_z;
	tmp_x = x + speed * t_x * time_elapsed;
	tmp_z = z + speed * t_z * time_elapsed;

	int row = tmp_z;
	int col = tmp_x;

	float row_result = tmp_z - row;
	float col_result = tmp_x - col;

	if (row_result >= 0.5f) row += 1;
	if (col_result >= 0.5f) col += 1;

	if (!IsCollied(row, col, Network::map)) {
		x = tmp_x;
		z = tmp_z;
	}
}

void Client::ProcessInput(float x, float y)
{
	if (abs(x) + abs(y) < 0.1f) {
		is_input = false;
		return;
	}
	is_input = true;
	
	XMFLOAT2 cur{ cos(angle), sin(angle) };
	XMFLOAT2 result;
	result.x = x * cur.x - y * cur.y;
	result.y = x * cur.x + y * cur.y;
	rotation_angle = atan2(result.y, result.x);
	/*rotation_angle = 0.0f;
	if (abs(angle) < rotate_speed) {
		rotation_angle = angle;
	}
	else {
		rotation_angle = angle / abs(angle) * rotate_speed;
	}
	float tmp_x = cos(rotation_angle);
	float tmp_y = sin(rotation_angle);
	t_x = tmp_x * t_x - tmp_y * t_z;
	t_z = tmp_x * t_x + tmp_y * t_z;*/
	/*XMStoreFloat2(&tmp_dir, XMVector2Normalize(XMLoadFloat2(&tmp_dir)));
	t_x = tmp_dir.x;
	t_z = tmp_dir.y;*/
}