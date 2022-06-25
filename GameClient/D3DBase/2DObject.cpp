#include "2DObject.h"
#include "Shader.h"
#include "GameFramework.h"

CMesh* C2DObject::mesh = NULL;

C2DObject::C2DObject()
{
	if (!mesh) mesh = C2DMesh::GetInstance();
	Scale(1000.0f);
}

void C2DObject::Animate(float fTimeElapsed)
{
	auto camera = CGameFramework::GetInstance()->GetCamera();
	auto target = CGameFramework::GetInstance()->GetCamera()->GetPosition();
	XMFLOAT3 up = CGameFramework::GetInstance()->GetCamera()->GetUpVector();
	/*XMFLOAT3 look = Vector3::Normalize(Vector3::Subtract(target, GetPosition()));
	XMFLOAT3 right = Vector3::CrossProduct(up, look, true);*/
	XMFLOAT3 look = camera->GetLookVector();
	XMFLOAT3 right = camera->GetRightVector();
	m_xmf4x4World._11 = right.x; m_xmf4x4World._12 = right.y; m_xmf4x4World._13 = right.z;
	m_xmf4x4World._21 = up.x; m_xmf4x4World._22 = up.y; m_xmf4x4World._23 = up.z;
	m_xmf4x4World._31 = look.x; m_xmf4x4World._32 = look.y; m_xmf4x4World._33 = look.z;
	
	//Rotate(fTimeElapsed * 10.0f, fTimeElapsed * 5.0f, fTimeElapsed * 2.0f);
	Scale(1000.0f);
}

void C2DObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (m_pMaterial && m_pMaterial->m_pTexture) {
		m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(2, m_d3dCbvGPUDescriptorHandle);
	
	if (mesh) mesh->Render(pd3dCommandList);
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}


void C2DObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (m_pMaterial && m_pMaterial->m_pTexture) {
		m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(2, desc_handle);

	if (mesh) mesh->Render(pd3dCommandList);
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}

CUIObject::CUIObject(float width, float height, float x, float y, CMaterial* material)
{
	Scale(width, height, 0);
	SetPosition(x, y, 0);
	SetMaterial(material);
}

void CUIObject::Animate(float fTimeElapsed)
{
}