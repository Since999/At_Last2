#pragma once
#include "Camera.h"

#define SUN_WIDTH 6000

#define SUN_HEIGHT 6000

struct CB_LIGHT_INFO {
	XMFLOAT4X4						shadow_transform;
	XMFLOAT4X4						shadow_view;
	XMFLOAT4X4						shadow_projection;
	XMFLOAT3						light_pos;
	XMFLOAT3						light_dir;
};

class CLight :
    public CCamera
{
	ID3D12Resource* cb_light = NULL;
	CB_LIGHT_INFO* mapped_light = NULL;
public:
	CLight();
	virtual void GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle);
	virtual void CLight::SetLookAt(XMFLOAT3& xmf3LookAt);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
};

class CSunLight : public CLight {
	XMFLOAT3 dir;
	float distance;
public:
	CSunLight(CPlayer* player);
	virtual void GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle);
	virtual void Update(XMFLOAT3& xmf3LookAt, float fTimeElapsed);
};
