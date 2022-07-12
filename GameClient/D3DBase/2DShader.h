#pragma once
#include "Shader.h"
#include "2DObject.h"
#include <map>
#include <string>
#define MAX_PARTICLE 10000
#define MAX_PARTICLE_TYPE 50

class C2DShader :
    public CObjectsShader
{
private:
	std::map<std::wstring, CTexture*> texture_map;
public:
	C2DShader(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig);
	C2DShader() {}
	~C2DShader();
	
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	//virtual void AnimateObjects(float fTimeElapsed);

	//virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual CTexture* GetTexture(const wstring& tex_file_name);
	virtual void AddTexture(ID3D12Device* device, CTexture* tex);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual D3D12_BLEND_DESC CreateBlendState();
};

#define MAX_UI 100
#define MAX_UI_TYPE 50

class C2DShaderSample : public C2DShader {
public:
	C2DShaderSample(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig);
	~C2DShaderSample() {}
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
};

class UISystem : public C2DShader {
public:
	UISystem(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig);
	~UISystem();

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	void AddUI(float width, float height, float x, float y, const wstring& image_file_name);
	void AddProgressBar(float width, float height, float x, float y, const wstring& image_file_name);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

private:
	ID3D12RootSignature* root_signagture = NULL;
	CCamera* camera = NULL;
};

class ParticleSystem : public C2DShader {
private:
	static ParticleSystem* singleton;
public:
	static ParticleSystem* GetInstance();
	static ParticleSystem* InitInstance(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig);
private:
	map<wstring, CParticleBuilder> particle_pool;
public:
	ParticleSystem(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig);
	~ParticleSystem();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void AddParticle(const XMFLOAT3& position, const wstring& name);
	void AddTrail(const XMFLOAT3& position, const wstring& name);
	void AddBuilder(const wstring& name, float duration, const XMFLOAT2& size, vector<CMaterial*>* materials);
private:
	void LoadParticle(const wstring& name);
};