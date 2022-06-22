#pragma once
#include "Shader.h"

#define MAX_PARTICLE 300
#define MAX_PARTICLE_TYPE 50

class C2DShader :
    public CObjectsShader
{
public:
	C2DShader(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig);
	virtual ~C2DShader();

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	//virtual void AnimateObjects(float fTimeElapsed);

	//virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void AddTexture(ID3D12Device* device, CTexture* tex);
	virtual void AddObject();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual D3D12_BLEND_DESC CreateBlendState();
};

//ROOT_PARAMATER_INDEX::GAMEOBJECT
