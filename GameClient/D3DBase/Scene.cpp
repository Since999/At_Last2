//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "CTerrainShader.h"

#include "CStaticObjectShader.h"
#include "CAnimationObject.h"
#include "ModelLoader.h"
#include "TexturePool.h"
#include "Configuration.h"
#include "AnimationShader.h"
#include "2DShader.h"

CScene::CScene()
{
}

CScene::~CScene()
{
}

void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	m_nShaders = 1;
	m_ppShaders = new CShader * [m_nShaders];

	/*CTerrainShader* pObjectShader = new CTerrainShader();
	pObjectShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	pObjectShader->BuildObjects(pd3dDevice, pd3dCommandList, NULL);
	m_ppShaders[0] = pObjectShader;

	CAnimationObjectShader* ani_shader = new CAnimationObjectShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_ppShaders[1] = ani_shader;

	CStaticObjectShader* static_shader = new CStaticObjectShader();
	static_shader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	static_shader->BuildObjects(pd3dDevice, pd3dCommandList, NULL);
	m_ppShaders[2] = static_shader;*/

	C2DShader* particle_shader = new C2DShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_ppShaders[0] = particle_shader;


	/*for(int i = 0; i< ROAD_ZOMBIE_NUM + FIRST_CHECK_POINT_ZOMBIE_NUM; ++i)
	
	{
		CZombie* object = new CZombie(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature,
			NULL);

		XMFLOAT3 pos = XMFLOAT3{ 50500.0f - 500.0f * i, CConfiguration::bottom, 14000.0f };
		object->SetPosition(pos);
		;
		if(i < ROAD_ZOMBIE_NUM) ((CZombie*)object)->SetZombie(&Network::r_zombie1[i]);
		else {
			int n = i - ROAD_ZOMBIE_NUM;
			if(n <= FIRST_CHECK_POINT_ZOMBIE_NUM) ((CZombie*)object)->SetZombie(&Network::b_zombie1[n]);
		}
		objects.push_back(object);
	}*/

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	if (m_ppShaders)
	{
		for (int i = 0; i < m_nShaders; i++)
		{
			m_ppShaders[i]->ReleaseShaderVariables();
			m_ppShaders[i]->ReleaseObjects();
			m_ppShaders[i]->Release();
		}
		delete[] m_ppShaders;
	}

	for (auto& object : object_list) {
		if (object) delete object;
	}
	object_list.clear();

	ReleaseShaderVariables();
}

void CScene::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[4];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 2; //GameObject
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 1;
	pd3dDescriptorRanges[1].BaseShaderRegister = 0; //t0: gtxtTexture
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	pd3dDescriptorRanges[2].NumDescriptors = 1;
	pd3dDescriptorRanges[2].BaseShaderRegister = 3; //AnimationObject
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 1;
	pd3dDescriptorRanges[3].BaseShaderRegister = 1; //t1: gShadowMap
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[(int)ROOT_PARAMATER_INDEX::SIZE];

	auto player = (int)ROOT_PARAMATER_INDEX::PLAYER;
	pd3dRootParameters[player].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[player].Descriptor.ShaderRegister = 0; //Player
	pd3dRootParameters[player].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[player].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto camera = (int)ROOT_PARAMATER_INDEX::CAMERA;
	pd3dRootParameters[camera].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[camera].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[camera].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[camera].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto object = (int)ROOT_PARAMATER_INDEX::GAMEOBJECT;
	pd3dRootParameters[object].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[object].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[object].DescriptorTable.pDescriptorRanges =
		&pd3dDescriptorRanges[0]; //GameObject
	pd3dRootParameters[object].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	auto texture = (int)ROOT_PARAMATER_INDEX::TEXTURE;
	pd3dRootParameters[texture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[texture].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[texture].DescriptorTable.pDescriptorRanges =
		&pd3dDescriptorRanges[1]; //Texture
	pd3dRootParameters[texture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	auto animation = (int)ROOT_PARAMATER_INDEX::ANIMATION;
	pd3dRootParameters[animation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[animation].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[animation].DescriptorTable.pDescriptorRanges =
		&pd3dDescriptorRanges[2];	//Animation
	pd3dRootParameters[animation].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto light = (int)ROOT_PARAMATER_INDEX::LIGHT;
	pd3dRootParameters[light].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[light].Descriptor.ShaderRegister = 4; //Light
	pd3dRootParameters[light].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[light].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto shadow_map = (int)ROOT_PARAMATER_INDEX::SHADOW_MAP;
	pd3dRootParameters[shadow_map].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[shadow_map].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[shadow_map].DescriptorTable.pDescriptorRanges =
		&pd3dDescriptorRanges[3]; //ShadowMap
	pd3dRootParameters[shadow_map].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplers[2];

	D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc;
	::ZeroMemory(&d3dSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
	d3dSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.MipLODBias = 0;
	d3dSamplerDesc.MaxAnisotropy = 1;
	d3dSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDesc.MinLOD = 0;
	d3dSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc.ShaderRegister = 0;
	d3dSamplerDesc.RegisterSpace = 0;
	d3dSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	samplers[0] = d3dSamplerDesc;

	d3dSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	d3dSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc.MipLODBias = 0.0f;
	d3dSamplerDesc.MaxAnisotropy = 16;
	d3dSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	d3dSamplerDesc.MinLOD = 0;
	d3dSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc.ShaderRegister = 1;
	d3dSamplerDesc.RegisterSpace = 0;
	d3dSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplers[1] = d3dSamplerDesc;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = _countof(samplers);
	d3dRootSignatureDesc.pStaticSamplers = samplers;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);

	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (int i = 0; i < m_nShaders; i++)
	{
		m_ppShaders[i]->UpdateShaderVariables(pd3dCommandList);
	}
	for (auto& object : object_list) {
		object->UpdateShaderVariables(pd3dCommandList);
	}
}

void CScene::ReleaseShaderVariables()
{
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return(false);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return(false);
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}
float ani_change_time;
void CScene::AnimateObjects(float fTimeElapsed)
{
	for (int i = 0; i < m_nShaders; i++)
	{
		m_ppShaders[i]->AnimateObjects(fTimeElapsed);
	}
	int n = 0;
	for (auto& object : object_list) {
		object->Animate(fTimeElapsed);
		n++;
	}
	/*ani_change_time += fTimeElapsed;
	if (ani_change_time > 2.f) {
		((CZombie*)objects[0])->ChangeAni();
		ani_change_time = 0.f;
	}*/
	for (auto& object : remove_list) {
		object_list.remove(object);
	}
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	//UpdateShaderVariables(pd3dCommandList);

	for (int i = 0; i < m_nShaders; i++)
	{
		m_ppShaders[i]->Render(pd3dCommandList, pCamera);
	}

	for (const auto& object : object_list) {
		object->Render(pd3dCommandList);
	}
}

void CScene::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	for (int i = 0; i < m_nShaders; i++)
	{
		m_ppShaders[i]->ShadowMapRender(pd3dCommandList);
	}
	for (const auto& object : object_list) {
		object->ShadowMapRender(pd3dCommandList);
	}
}

void CScene::AddObject(CGameObject* object)
{
	object_list.push_back(object);
}

void CScene::RemoveObject(CGameObject* object)
{
	remove_list.push_back(object);
}

void CScene::RemoveObjects()
{
	for (const auto& object : remove_list) {
		auto found = find(object_list.begin(), object_list.end(), object);
		if (found == object_list.end()) {
#ifdef _DEBUG
			cout << "warning(CObjectShader::RemoveObject): object not found" << endl;
#endif
			return;
		}
		delete (*found);
		object_list.erase(found);
	}
	remove_list.clear();
}