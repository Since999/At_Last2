#pragma once
#include "Shader.h"
#include <map>

class CAnimationObject;
class Zombie;
struct CB_ANIMATION_OBJECT_INFO;

class CAnimationShader :
	public CShader
{
public:
	CAnimationShader();
	virtual ~CAnimationShader();

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

};

class CAnimationObjectShader : public CAnimationShader
{
public:
	static CAnimationObjectShader* GetInstance() { return singleton; }
private:
	static CAnimationObjectShader* singleton;
public:
	CAnimationObjectShader(ID3D12Device* device, ID3D12GraphicsCommandList* com_list, ID3D12RootSignature* root_sig);
	~CAnimationObjectShader();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	virtual void AnimateObjects(float fTimeElapsed);
	void RemoveObjects();
	virtual void ReleaseObjects();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void ReleaseUploadBuffers();

	virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void AddZombie(Zombie*);
	virtual void DeleteZombie(CGameObject* object);

	virtual void AddZombieInNetwork();
	template<class Arr>
	void CheckZombies(Arr arr);

protected:
	std::list<CAnimationObject*> objects;
	vector<CGameObject*> remove_list;

	unsigned int max_object_num = 400;

	ID3D12Resource* m_pd3dcbGameObjects = NULL;
	CB_ANIMATION_OBJECT_INFO* m_pcbMappedGameObjects = NULL;

	CShadowShader* shadow_shader = NULL;
	map<ZombieType, pair<string, wstring>> zombie_model_map;

	const map<ZombieType, pair<string, wstring>>& GetModelMap();
};