#pragma once
#include "Shader.h"

class SkinModel;
class CShadowShader;

class CTerrainShader : public CTexturedShader
{
private:
	CShadowShader* shadow_shader = NULL;
public:
	CTerrainShader();
	virtual ~CTerrainShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void ReleaseObjects();

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList);
protected:
	CGameObject** m_ppObjects = 0;
	
	SkinModel* model;

	int								m_nObjects = 0;

	ID3D12Resource* m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO* m_pcbMappedGameObjects = NULL;
};

