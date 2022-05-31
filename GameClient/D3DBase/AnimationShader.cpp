#include "AnimationShader.h"
#include "ShadowShader.h"
#include "CAnimationObject.h"

CAnimationObjectShader* CAnimationObjectShader::singleton = NULL;

CAnimationShader::CAnimationShader()
{
}

CAnimationShader::~CAnimationShader()
{
}

void CAnimationShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CAnimationShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
}

D3D12_INPUT_LAYOUT_DESC CAnimationShader::CreateInputLayout()
{

	UINT nInputElementDescs = 5;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, 12 + 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 12 + 12 + 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT,		0, 12 + 12 + 8 + 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CAnimationShader::CreateVertexShader(ID3DBlob** ppd3dShaderBlob)
{
	//return(CShader::CompileShaderFromFile(L"Animation.hlsl", "VSAnimated", "vs_5_1", ppd3dShaderBlob));
	return(CShader::CompileShaderFromFile(L"PlayerShader.hlsl", "PlayerVS", "vs_5_1", ppd3dShaderBlob));
}

D3D12_SHADER_BYTECODE CAnimationShader::CreatePixelShader(ID3DBlob** ppd3dShaderBlob)
{
	//return(CShader::CompileShaderFromFile(L"Animation.hlsl", "PSAnimated", "ps_5_1", ppd3dShaderBlob));
	return(CShader::CompileShaderFromFile(L"PlayerShader.hlsl", "PlayerPS", "ps_5_1", ppd3dShaderBlob));
}

void CAnimationShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	CShader::CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
}



//-----------------------------------------------------------------------------------------------------------------------

CAnimationObjectShader::CAnimationObjectShader(ID3D12Device* device, ID3D12GraphicsCommandList* com_list, ID3D12RootSignature* root_sig)
{
	CreateShader(device, root_sig);
	BuildObjects(device, com_list);
	singleton = this;
}

CAnimationObjectShader::~CAnimationObjectShader()
{
}

void CAnimationObjectShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	CAnimationShader::CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	shadow_shader = new CAnimationShadowShader();
	shadow_shader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
}

D3D12_SHADER_BYTECODE CAnimationObjectShader::CreateVertexShader(ID3DBlob** ppd3dShaderBlob)
{
	//return(CShader::CompileShaderFromFile(L"Animation.hlsl", "VSAnimated", "vs_5_1", ppd3dShaderBlob));
	return(CShader::CompileShaderFromFile(L"Animation.hlsl", "VSAnimated", "vs_5_1", ppd3dShaderBlob));
}

D3D12_SHADER_BYTECODE CAnimationObjectShader::CreatePixelShader(ID3DBlob** ppd3dShaderBlob)
{
	//return(CShader::CompileShaderFromFile(L"Animation.hlsl", "PSAnimated", "ps_5_1", ppd3dShaderBlob));
	return(CShader::CompileShaderFromFile(L"Animation.hlsl", "PSAnimated", "ps_5_1", ppd3dShaderBlob));
}

void CAnimationObjectShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_ANIMATION_OBJECT_INFO) + 255) & ~255); //256ÀÇ ¹è¼ö
	m_pd3dcbGameObjects = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes * max_object_num, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

}

void CAnimationObjectShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dcbGameObjects->Map(0, NULL, (void**)&m_pcbMappedGameObjects);
	UINT ncbElementBytes = ((sizeof(CB_ANIMATION_OBJECT_INFO) + 255) & ~255);
	int i = 0;
	for (auto& [key, object] : objects) {
		CB_ANIMATION_OBJECT_INFO* pbMappedcbGameObject = (CB_ANIMATION_OBJECT_INFO*)((UINT8*)m_pcbMappedGameObjects + (i * ncbElementBytes));
		object->UpdateShaderVariables(pd3dCommandList, pbMappedcbGameObject);
		++i;
	}
	m_pd3dcbGameObjects->Unmap(0, NULL);
}

void CAnimationObjectShader::ReleaseShaderVariables()
{
	if (m_pd3dcbGameObjects)
	{
		m_pd3dcbGameObjects->Unmap(0, NULL);
		m_pd3dcbGameObjects->Release();
	}

	CAnimationShader::ReleaseShaderVariables();
}


#include "OldModelLoader.h"
#include "TexturePool.h"
void CAnimationObjectShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	CreateCbvSrvDescriptorHeaps(pd3dDevice, max_object_num, 0);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, max_object_num, m_pd3dcbGameObjects, ((sizeof(CB_ANIMATION_OBJECT_INFO) + 255) & ~255));
	//+ FIRST_CHECK_POINT_ZOMBIE_NUM;
	for(int i = 0; i< ROAD_ZOMBIE_NUM + FIRST_CHECK_POINT_ZOMBIE_NUM; ++i)
	{
		Zombie* zom = NULL;
		if(i < ROAD_ZOMBIE_NUM) zom = &Network::r_zombie1[i];
		else {
			int n = i - ROAD_ZOMBIE_NUM;
			if(n <= FIRST_CHECK_POINT_ZOMBIE_NUM) zom = &Network::b_zombie1[n];
		}
		zom->_id = i;
		AddZombie(zom);
	}
}

void CAnimationObjectShader::ReleaseObjects()
{
	for (auto& [key, object] : objects) {
		delete object;
	}
}

void CAnimationObjectShader::AnimateObjects(float fTimeElapsed)
{
	AddZombieInNetwork();
	for (auto& [key, object] : objects) {
		object->Animate(fTimeElapsed);
	}
}

void CAnimationObjectShader::ReleaseUploadBuffers()
{
	for (auto& [key, object] : objects) {
		object->ReleaseUploadBuffers();
	}
}

void CAnimationObjectShader::OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pd3dPipelineState) pd3dCommandList->SetPipelineState(m_pd3dPipelineState);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	//UpdateShaderVariables(pd3dCommandList);
}

void CAnimationObjectShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CAnimationShader::Render(pd3dCommandList, pCamera);
	int i = 0;
	for (auto& [key, object] : objects) {
		object->Render(pd3dCommandList, m_pd3dCbvSrvDescriptorHeap, m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * i), pCamera);
		++i;
	}
}

void CAnimationObjectShader::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList) {
	shadow_shader->Render(pd3dCommandList, NULL);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);
	int i = 0;
	for (auto& [key, object] : objects) {
		object->ShadowMapRender(pd3dCommandList, m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * i));
		++i;
	}
}
#include "Configuration.h"
void CAnimationObjectShader::AddZombie(Zombie* zombie)
{
	zombie->_id;
	CZombie* object = new CZombie();
#ifdef ENABLE_NETWORK
	XMFLOAT3 pos = XMFLOAT3{ 100500.0f, CConfiguration::bottom, 14000.0f };
#else
	XMFLOAT3 pos = XMFLOAT3{ 50500.0f - 500.0f * objects.size(), CConfiguration::bottom, 14000.0f };
#endif
	object->SetPosition(pos);
	
	object->SetZombie(zombie);
	
	objects.try_emplace(zombie->_id, object);
}

void CAnimationObjectShader::DeleteZombie(int num)
{
	auto& found = objects.find(num);
	if (found != objects.end())
	{
		delete (*found).second;
		objects.erase(num);
	}
}

void CAnimationObjectShader::AddZombieInNetwork()
{
	
	auto& zombies = Network::r_zombie1;
	for (auto& zombie : zombies)
	{
		if (zombie._state == ZombieState::FREE || zombie._state == ZombieState::SLEEP) continue;
		AddZombie(&zombie);
	}
	
	Network::r_zombie2;
	Network::r_zombie3;
	Network::b_zombie1;
	Network::b_zombie2;
	Network::b_zombie3;
}

template<class Arr>
void CheckZombies(Arr arr){
	Network::r_zombie1;
	int i = 0;
}