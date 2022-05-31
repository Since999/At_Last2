#include "CLight.h"
#include "Player.h"

CLight::CLight() : CCamera() {
	SetOffset(XMFLOAT3(0.0f, 000.0f, -5000.0f));
	GenerateProjectionMatrix(1.01f, 1000000.0f, ASPECT_RATIO, 60.0f);

	CreateShaderVariables(DEVICEMANAGER.pd3dDevice, DEVICEMANAGER.pd3dCommandList);
}

void CLight::GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle)
{
	m_xmf4x4Projection = Matrix4x4::PerspectiveFovLH(XMConvertToRadians(fFOVAngle), fAspectRatio, fNearPlaneDistance, fFarPlaneDistance);
}

void CLight::SetLookAt(XMFLOAT3& xmf3LookAt)
{
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(m_xmf3Position, xmf3LookAt, m_pPlayer->GetUpVector());
	m_xmf3Right = XMFLOAT3(mtxLookAt._11, mtxLookAt._21, mtxLookAt._31);
	m_xmf3Up = XMFLOAT3(mtxLookAt._12, mtxLookAt._22, mtxLookAt._32);
	m_xmf3Look = XMFLOAT3(mtxLookAt._13, mtxLookAt._23, mtxLookAt._33);
}

void CLight::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CCamera::CreateShaderVariables(pd3dDevice, pd3dCommandList);
	UINT ncbElementBytes = ((sizeof(CB_LIGHT_INFO) + 255) & ~255); //256ÀÇ ¹è¼ö
	cb_light = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
}

void CLight::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	CCamera::UpdateShaderVariables(pd3dCommandList);
	cb_light->Map(0, NULL, (void**)&mapped_light);
	XMMATRIX T{
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	};
	
	XMMATRIX S = XMLoadFloat4x4(&m_xmf4x4View) * XMLoadFloat4x4(&m_xmf4x4Projection) * T;
	XMStoreFloat3(&mapped_light->light_dir, XMLoadFloat3(&Vector3::Normalize(m_xmf3Look)));
	XMStoreFloat3(&mapped_light->light_pos, XMLoadFloat3(&m_xmf3Position));
	XMStoreFloat4x4(&mapped_light->shadow_transform, XMMatrixTranspose(S));
	XMStoreFloat4x4(&mapped_light->shadow_view, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4View)));
	XMStoreFloat4x4(&mapped_light->shadow_projection, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4Projection)));
	cb_light->Unmap(0, NULL);
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = cb_light->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(5, d3dGpuVirtualAddress);
}

//----------------------------------------------------------------------------------------------------------------------------------------------
//CSunLight

CSunLight::CSunLight(CPlayer* player) : CLight()
{
	m_pPlayer = player;
	if (player) {
		SetLookAt(player->GetPosition());
	}
	dir = Vector3::Normalize(XMFLOAT3( 1.0f, 1.0f, -0.1f ));
	distance = 3000.0;
	//SetOffset(XMFLOAT3(0.0f, 000.0f, -5000.0f));
	GenerateProjectionMatrix(1.01f, 1000000.0f, ASPECT_RATIO, 60.0f);
	CreateShaderVariables(DEVICEMANAGER.pd3dDevice, DEVICEMANAGER.pd3dCommandList);
}

void CSunLight::GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle)
{
	//sun
	XMStoreFloat4x4(&m_xmf4x4Projection, XMMatrixOrthographicLH(SUN_WIDTH, SUN_HEIGHT, 1.0f, 10000.0f));
	//m_xmf4x4Projection = Matrix4x4::PerspectiveFovLH(XMConvertToRadians(fFOVAngle), fAspectRatio, fNearPlaneDistance, fFarPlaneDistance);
}

void CSunLight::Update(XMFLOAT3& xmf3LookAt, float fTimeElapsed)
{
	if (m_pPlayer)
	{
		XMFLOAT3 offset = Vector3::ScalarProduct(dir, distance, false);
		XMFLOAT3 xmf3Position = Vector3::Add(m_pPlayer->GetPosition(), offset);
		m_xmf3Position = Vector3::Add(m_pPlayer->GetPosition(), offset);
		SetLookAt(m_pPlayer->GetPosition());
		RegenerateViewMatrix();
	}
}

