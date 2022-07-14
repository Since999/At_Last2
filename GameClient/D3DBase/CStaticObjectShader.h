#pragma once
#include "Shader.h"

#define MAX_ADDITIONAL_OBJECT 100

class CStaticObjectShader : public CTexturedShader
{
	static CStaticObjectShader* singleton;
public:
	static CStaticObjectShader* GetInstance();
	static CStaticObjectShader* InitInstance(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	CStaticObjectShader();
	virtual ~CStaticObjectShader();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void ReleaseObjects();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void AddBarricade(const BarricadePos& barricade);
protected:
	vector<CStaticObject*> objects;
	unsigned int object_num;

	std::vector<CMesh*> mesh;

	ID3D12Resource* m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO* m_pcbMappedGameObjects = NULL;

	CShadowShader* shadow_shader = NULL;

	UINT64 cb_size = 0;

};