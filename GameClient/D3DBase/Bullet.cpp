#include "Bullet.h"
#include "2DShader.h"
#include "GameFramework.h"

CBullet::CBullet(const XMFLOAT3& position, const XMFLOAT3& direction, float speed )
	:direction(direction), speed(speed)
{
	SetPosition(position);
}

void CBullet::Animate(float fTimeElapsed)
{
	//move
	auto& pre_pos = GetPosition();
	auto& dir = Vector3::ScalarProduct(direction, speed * fTimeElapsed, false);
	auto& cur_pos = Vector3::Add(pre_pos, dir);
	SetPosition(cur_pos);
	
	//make trail
	float interval = 5;
	auto& sub = Vector3::Subtract(cur_pos, pre_pos);
	int len = (int)(Vector3::Length(sub) / interval);
	sub = Vector3::Normalize(sub);

	for (int i = 0; i < len; ++i) {
		XMFLOAT3 pos = Vector3::Add(pre_pos, Vector3::ScalarProduct(sub, i * 5, false));
		float rand_val = ((float)rand() / (float)RAND_MAX) - 0.5f;
		XMFLOAT3 right = Vector3::CrossProduct(direction, XMFLOAT3( 0.0f, 1.0f, 0.0f ));
		XMFLOAT3 random_pos = Vector3::ScalarProduct(right, rand_val * 20, false);
		pos = Vector3::Add(pos, random_pos);
		ParticleSystem::GetInstance()->AddTrail(pos, L"smoke");
	}


	time += fTimeElapsed;
	if (time > 0.2f) {
		CGameFramework::GetInstance()->GetCurruntScene()->RemoveObject(this);
	}
}