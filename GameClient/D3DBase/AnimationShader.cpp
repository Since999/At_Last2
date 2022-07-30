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
	for (auto& object : objects) {
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
#include "ModelManager.h"
#include "StateMachine.h"
void CAnimationObjectShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	CreateCbvSrvDescriptorHeaps(pd3dDevice, max_object_num, 0);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, max_object_num, m_pd3dcbGameObjects, ((sizeof(CB_ANIMATION_OBJECT_INFO) + 255) & ~255));
	//+ FIRST_CHECK_POINT_ZOMBIE_NUM;
	
	for (const auto& [a, info] : GetModelMap()) {
		auto model = ModelManager::GetModel(info.model, info.texture);
		model->SetAniRepeat(CZombieStateMachine::ANIMATION_INDEX::ATTACKED, false);
		model->SetAniRepeat(CZombieStateMachine::ANIMATION_INDEX::ATTACK, false);
		model->SetAniRepeat(CZombieStateMachine::ANIMATION_INDEX::DEAD, false);
		model->SetAniRepeat(CZombieStateMachine::ANIMATION_INDEX::SPAWN, false);
	}
	//for (int i = 0; i < ROAD_ZOMBIE_NUM + FIRST_CHECK_POINT_ZOMBIE_NUM; ++i)
	////for (int i = 0; i < 3; ++i)
	//{
	//	Zombie* zom = NULL;
	//	if(i < ROAD_ZOMBIE_NUM) zom = &Network::r_zombie1[i];
	//	else {
	//		int n = i - ROAD_ZOMBIE_NUM;
	//		if(n <= FIRST_CHECK_POINT_ZOMBIE_NUM) zom = &Network::b_zombie1[n];
	//	}
	//	//zom->_id = i;
	//	//zom->_type = (ZombieType)(((int)zom->_type + i) % 5);
	//	AddZombie(zom);
	//}
}

void CAnimationObjectShader::ReleaseObjects()
{
	for (auto& object : objects) {
		delete object;
	}
}

void CAnimationObjectShader::AnimateObjects(float fTimeElapsed)
{
	//AddZombieInNetwork();
	for (auto& object : objects) {
		object->Animate(fTimeElapsed);
	}
	RemoveObjects();
}

void CAnimationObjectShader::RemoveObjects()
{
	for (auto& object : remove_list) {
		auto& found = find(objects.begin(), objects.end(), object);
		if (found != objects.end())
		{
			delete (*found);
			objects.erase(found);
		}
	}
	remove_list.clear();
}

void CAnimationObjectShader::ReleaseUploadBuffers()
{
	for (auto& object : objects) {
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
	for (auto& object : objects) {
		object->Render(pd3dCommandList, m_pd3dCbvSrvDescriptorHeap, m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * i), pCamera);
		++i;
	}
}

void CAnimationObjectShader::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList) {
	shadow_shader->Render(pd3dCommandList, NULL);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);
	int i = 0;
	for (auto& object : objects) {
		object->ShadowMapRender(pd3dCommandList, m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * i));
		++i;
	}
}
/*
z1 - zombie
z2 zomie_goul_nomal
z3 - pumpkinHulk_diffuse
*/
const map<ZombieType, ZombieInfo>& CAnimationObjectShader::GetModelMap()
{
	if (!zombie_model_map.empty()) return zombie_model_map;
	ZombieInfo info;
	info.model = "Z1.fbx"; info.texture = L"Zombie.png"; info.size = 1.0f;
	zombie_model_map.emplace(ZombieType::NORMAL, info);
	//Test
	info.model = "Z2.fbx"; info.texture = L"zombie_goul_nomal.png"; info.size = 3.0f;
	zombie_model_map.emplace(ZombieType::SOLIDEIR, info);
	//Test
	info.model = "Z3.fbx"; info.texture = L"pumpkinHulk_diffuse.png"; info.size = 0.5f;
	zombie_model_map.emplace(ZombieType::TANKER, info);
	info.model = "z4_dog.fbx"; info.texture = L"Zombie.png"; info.size = 1.0f;
	zombie_model_map.emplace(ZombieType::DOG, info);
	return zombie_model_map;
}

#include "Configuration.h"
void CAnimationObjectShader::AddZombie(Zombie* zombie)
{
	auto& model_map = GetModelMap();
	CZombie* object;
	auto& found = model_map.find(zombie->_type);
	if (found == model_map.end()) {
		object = new CZombie();
	}
	else {
		auto& info = (*found).second;
		object = new CZombie(info.model, info.texture);
		object->SetSize(object->GetSize() * info.size);
	}
#ifdef ENABLE_NETWORK
	XMFLOAT3 pos = XMFLOAT3{ 100500.0f, CConfiguration::bottom, 14000.0f };	
#else
	XMFLOAT3 pos = XMFLOAT3{ 50500.0f - 500.0f * objects.size(), CConfiguration::bottom, 14000.0f };
#endif
	object->SetPosition(pos);
	
	object->SetZombie(zombie);
	
	objects.push_back(object);
}

void CAnimationObjectShader::DeleteZombie(CGameObject* object)
{
	remove_list.push_back(object);
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

CAnimationObject* CAnimationObjectShader::GetFirstZombie()
{
	if (objects.empty()) return NULL;
	return *(objects.begin());
}