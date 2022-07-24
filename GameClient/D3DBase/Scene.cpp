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
#include "SoundSystem.h"
#include "GameFramework.h"
#include "CLight.h"
CScene::CScene(ID3D12RootSignature* root_sig) : m_pd3dGraphicsRootSignature(root_sig)
{
}

CScene::~CScene()
{
	ReleaseObjects();
	StopEvent();
}

void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!m_pd3dGraphicsRootSignature) { 
		m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice); 
	}

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::ReleaseObjects()
{
	//if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

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

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[5];

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
	pd3dRootParameters[object].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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

bool CScene::ProcessInput(UCHAR* pKeysBuffer, HWND& hwnd)
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
	if (pCamera) {
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);
	}
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CLobbyScene

CLobbyScene::CLobbyScene(ID3D12RootSignature* root_sig) : CScene(root_sig)
{

}

CLobbyScene::~CLobbyScene()
{

}

void CLobbyScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CScene::BuildObjects(pd3dDevice, pd3dCommandList);
	CGameFramework::GetInstance()->ChangeUI(new UISystem(pd3dDevice, pd3dCommandList, GetGraphicsRootSignature(), "Resources/UI/lobby_UI.xml"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CLobbyScene

CSelectScene::CSelectScene(ID3D12RootSignature* root_sig): CScene(root_sig)
{

}

void CSelectScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CScene::BuildObjects(pd3dDevice, pd3dCommandList);
	CGameFramework::GetInstance()->ChangeUI(new UISystem(pd3dDevice, pd3dCommandList, GetGraphicsRootSignature(), "Resources/UI/lobby_UI_2.xml"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MainGameScene

CMainGameScene::CMainGameScene(ID3D12RootSignature* root_sig) : CScene(root_sig)
{	
}

void CMainGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* list)
{
	CScene::BuildObjects(device, list);

	CGameFramework* framework = CGameFramework::GetInstance();

	m_nShaders = 3;
	m_ppShaders = new CShader * [m_nShaders];

	/*CTerrainShader* pObjectShader = new CTerrainShader();
	pObjectShader->CreateShader(device, m_pd3dGraphicsRootSignature);
	pObjectShader->BuildObjects(device, list, NULL);
	m_ppShaders[0] = pObjectShader;*/

	CAnimationObjectShader* ani_shader = new CAnimationObjectShader(device, list, m_pd3dGraphicsRootSignature);
	//TEST
	test_zombie = ani_shader->GetFirstZombie();
	//TEST
	m_ppShaders[0] = ani_shader;


	m_ppShaders[1] = CStaticObjectShader::InitInstance(device, list, m_pd3dGraphicsRootSignature);

	C2DShader* particle_shader = new C2DShader(device, list, m_pd3dGraphicsRootSignature);
	m_ppShaders[2] = particle_shader;

	framework->ChangeUI(new UISystem(device, list, GetGraphicsRootSignature(), "Resources/UI/TestUI.xml"));

	unsigned int model_index;
#ifndef ENABLE_NETWORK
	network.g_client[network.my_id]._type = PlayerType::COMMANDER;
#endif
	model_index = (int)(network.g_client[network.my_id]._type);
	m_pPlayer = new CMainGamePlayer(device, list, GetGraphicsRootSignature(), NULL, 10,
		CConfiguration::player_models[model_index].model, CConfiguration::player_models[model_index].texture.c_str());
	model_index = (int)(network.g_client[network.other_client_id1]._type);
	m_pPlayer2 = new CMainGamePlayer(device, list, GetGraphicsRootSignature(), NULL, 10,
		CConfiguration::player_models[model_index].model, CConfiguration::player_models[model_index].texture.c_str());
	model_index = (int)(network.g_client[network.other_client_id2]._type);
	m_pPlayer3 = new CMainGamePlayer(device, list, GetGraphicsRootSignature(), NULL, 10,
		CConfiguration::player_models[model_index].model, CConfiguration::player_models[model_index].texture.c_str());


	float bottom = -580.0;
#ifdef ENABLE_NETWORK
	m_pPlayer->SetPosition({ (network.g_client[network.my_id].Get_Client_X() - 550.0f) * (-100.0f), 00.0f, (network.g_client[network.my_id].Get_Client_Z() - 210.0f) * (-100.0f) });

	m_pPlayer->SetPosition({ 50500.0f, bottom, 14000.0f });
	m_pPlayer2->SetPosition({ (network.g_client[network.other_client_id1].Get_Client_X() - 550.0f) * (-100.0f), 00.0f, (network.g_client[network.other_client_id1].Get_Client_Z() - 210.0f) * (-100.0f) });
	m_pPlayer3->SetPosition({ (network.g_client[network.other_client_id2].Get_Client_X() - 550.0f) * (-100.0f), 00.0f, (network.g_client[network.other_client_id2].Get_Client_Z() - 210.0f) * (-100.0f) });

	framework->client_player = m_pPlayer;
#else
	m_pPlayer->SetPosition({ 50500.0f, bottom, 14000.0f });
	m_pPlayer2->SetPosition({ 50500.0f, bottom, 14000.0f });
	m_pPlayer3->SetPosition({ 50500.0f, bottom, 14000.0f });
	client_player = m_pPlayer;
#endif
	((CMainGamePlayer*)m_pPlayer)->SetPlayerInfo(&network.g_client[network.my_id]);
	((CMainGamePlayer*)m_pPlayer2)->SetPlayerInfo(&network.g_client[network.other_client_id1]);
	((CMainGamePlayer*)m_pPlayer3)->SetPlayerInfo(&network.g_client[network.other_client_id2]);

	client_player->ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
	framework->m_pCamera = client_player->GetCamera();

	framework->m_pCamera->CreateShaderVariables(device, list);

	sun_light = new CSunLight(m_pPlayer);
}

CMainGameScene::~CMainGameScene()
{
	ReleaseObjects();
}

void CMainGameScene::ReleaseObjects()
{
	CScene::ReleaseObjects();
	ReleaseShaderVariables();

	if (m_pPlayer) delete m_pPlayer;
	if (m_pPlayer2) delete m_pPlayer2;
	if (m_pPlayer3) delete m_pPlayer3;
}

void CMainGameScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CScene::CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CMainGameScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	CScene::UpdateShaderVariables(pd3dCommandList);
	m_pPlayer2->UpdateShaderVariables(pd3dCommandList);
	m_pPlayer3->UpdateShaderVariables(pd3dCommandList);
	m_pPlayer->UpdateShaderVariables(pd3dCommandList);
}

void CMainGameScene::ReleaseShaderVariables()
{
	CScene::ReleaseShaderVariables();
	m_pPlayer2->ReleaseShaderVariables();
	m_pPlayer3->ReleaseShaderVariables();
	m_pPlayer->ReleaseShaderVariables();
}

void CMainGameScene::AnimateObjects(float fTimeElapsed)
{
	CScene::AnimateObjects(fTimeElapsed);
	m_pPlayer2->Update(fTimeElapsed);
	m_pPlayer3->Update(fTimeElapsed);
	m_pPlayer->Update(fTimeElapsed);

	sun_light->Update(XMFLOAT3(), fTimeElapsed);
}

//#define _WITH_PLAYER_TOP

void CMainGameScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CScene::Render(pd3dCommandList, pCamera);
#ifdef _WITH_PLAYER_TOP
	pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
	m_pPlayer->Render(pd3dCommandList, pCamera);
	m_pPlayer2->Render(pd3dCommandList, pCamera);
	m_pPlayer3->Render(pd3dCommandList, pCamera);

}
void CMainGameScene::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	sun_light->UpdateShaderVariables(pd3dCommandList);

	//Render
	CScene::ShadowMapRender(pd3dCommandList);
	m_pPlayer->ShadowMapRender(pd3dCommandList);
	m_pPlayer2->ShadowMapRender(pd3dCommandList);
	m_pPlayer3->ShadowMapRender(pd3dCommandList);
}

bool CMainGameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		((CMainGamePlayer*)client_player)->StartFire();

		//TEST
		test_zombie->ChangeAni();
		//TEST
#ifdef TEST
		POINT point;
		::GetCursorPos(&point);
		ScreenToClient(hWnd, &point);

		RECT rect;
		GetClientRect(hWnd, &rect);
		float width = rect.right - rect.left;
		float height = rect.bottom - rect.top;
		float x = point.x - (width / 2);
		float y = point.y - (height / 2);
		x = -x;
		((CMainGamePlayer*)client_player)->MoveTo(x, y);
#endif
	}
	break;
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
		((CMainGamePlayer*)client_player)->StopFire();
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
	return(false);
}

bool CMainGameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'R':
			((CMainGamePlayer*)client_player)->Reload();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return(false);
}

bool CMainGameScene::ProcessInput(UCHAR* pKeysBuffer, HWND& hwnd)
{
	DWORD dwDirection = 0;

	if ((pKeysBuffer['W'] & 0xF0)) dwDirection |= DIR_FORWARD;
	if ((pKeysBuffer['S'] & 0xF0)) dwDirection |= DIR_BACKWARD;
	if ((pKeysBuffer['A'] & 0xF0)) dwDirection |= DIR_LEFT;
	if ((pKeysBuffer['D'] & 0xF0)) dwDirection |= DIR_RIGHT;
	if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
	if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;
#ifdef _DEBUG
	auto camera = CGameFramework::GetInstance()->GetCamera();
	if (pKeysBuffer[VK_OEM_PLUS] & 0xF0 || pKeysBuffer[VK_ADD] & 0xF0) {
		if (camera) camera->AddDistance(50.0f);
	}
	if (pKeysBuffer[VK_OEM_MINUS] & 0xF0 || pKeysBuffer[VK_SUBTRACT] & 0xF0) {
		if (camera) camera->AddDistance(-50.0f);
	}
#endif
#ifdef ENABLE_NETWORK
	if ((pKeysBuffer['E'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_PLAYER_INTERATION);
	}
	// 채팅용 키 엔터 또는 등등을 만들어야 할 수도 있음 현재는 만들지 않음

	if ((pKeysBuffer[VK_LSHIFT] & 0XF0) && network.key_down_state == false && network.g_client[network.my_id].special_skill > 0)
	{
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_PLAYER_SPECIAL);
	}

	if ((pKeysBuffer['C'] & 0xF0) && network.g_client[network.my_id].special_skill_key == true)
	{
		network.g_client[network.my_id].special_skill_key = false;
		network.Send_commander_special_req_packet(network.g_client[network.my_id].special_id);
	}

	if ((pKeysBuffer['V'] & 0xF0) && network.g_client[network.my_id].special_skill_key == true)
	{
		network.g_client[network.my_id].special_skill_key = false;
		network.Send_commander_special_req_packet(network.g_client[network.my_id].special_id);
	}

	if ((pKeysBuffer['I'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_MAP_CHANGE_ROAD_ONE);
	}

	if ((pKeysBuffer['O'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_MAP_CHANGE_ROAD_TWO);
	}

	if ((pKeysBuffer['P'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_MAP_CHANGE_ROAD_THREE);
	}

	if ((pKeysBuffer['J'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_MAP_CHANGE_BASE_ONE);
	}

	if ((pKeysBuffer['K'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_MAP_CHANGE_BASE_TWO);
	}

	if ((pKeysBuffer['L'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_MAP_CHANGE_BASE_THREE);
	}

	if ((pKeysBuffer['N'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_ZOMBIE_ALL_KILL);
	}

	if ((pKeysBuffer['M'] & 0xF0) && network.key_down_state == false) {
		network.key_down_state = true;
		network.Send_request_packet(MsgType::CS_GM_PLAYER_HP_UP);
	}
#endif

	float cxDelta = 0.0f, cyDelta = 0.0f;
	POINT ptCursorPos;
	//if (GetCapture() == m_hWnd)
	{
		GetCursorPos(&ptCursorPos);
		ScreenToClient(hwnd, &ptCursorPos);
		RECT rect;
		GetClientRect(hwnd, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		XMVECTOR cursor_direction = { ptCursorPos.x - (width / 2), ptCursorPos.y - (height / 2) };
		cursor_direction = XMVector2Normalize(cursor_direction);
		network.g_client[network.my_id].mx = cursor_direction.m128_f32[0];
		network.g_client[network.my_id].mz = cursor_direction.m128_f32[1];
		
	}

#ifdef ENABLE_NETWORK
	if (network.mouse_state == false)
	{
		network.mouse_state = true;
		network.Send_rotate_packet(network.g_client[network.my_id].mx, network.g_client[network.my_id].mz);
	}

	if (GetCapture() == m_hWnd && network.attack_state == false)
	{
		network.attack_state = true;
		int m_x = ptCursorPos.x - 640;
		int m_z = ptCursorPos.y - 360;
		network.Send_attack_packet(m_x, m_z);
	}
#endif
	if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
	{
		if (cxDelta || cyDelta)
		{
		}
		if (dwDirection) client_player->Move(dwDirection, 200.0f * TIMEMANAGER.GetTimeElapsed(), true);
	}
	{
		XMVECTOR direction = { 0.0f, 0.0f };

		if (dwDirection & DIR_FORWARD) direction += {1.0f, 0.0f};
		if (dwDirection & DIR_BACKWARD) direction += {-1.0f, 0.0f};
		if (dwDirection & DIR_RIGHT) direction += {0.0f, -1.0f};
		if (dwDirection & DIR_LEFT) direction += {0.0f, 1.0f};
		direction = XMVector2Normalize(direction);

		XMFLOAT2 dest;
		XMStoreFloat2(&dest, direction);
		network.g_client[network.my_id].ProcessInput(dest.x, dest.y);
	}
	return(false);
}

void CMainGameScene::StartEvent()
{
	CSoundSystem::GetInstance()->PlayBGM(L"main game bgm");
}

void CMainGameScene::StopEvent()
{
	CSoundSystem::GetInstance()->StopBGM();
}