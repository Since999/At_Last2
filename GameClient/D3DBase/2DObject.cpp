#include "2DObject.h"
#include "Shader.h"

C2DObject::C2DObject(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist)
{
	m_nMeshes = 1;
	m_ppMeshes = new CMesh*[m_nMeshes];
	m_ppMeshes[0] = new C2DMesh(device, cmdlist);
	Scale(1000.0f);
}

void C2DObject::Animate(float fTimeElapsed)
{
	Rotate(fTimeElapsed * 10.0f, fTimeElapsed * 5.0f, fTimeElapsed * 2.0f);
	//Scale(1000000000.0f);
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
	
	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList);
		}
	}
}