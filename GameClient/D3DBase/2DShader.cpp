#include "2DShader.h"
#include "2DObject.h"
#include "GameFramework.h"
#include "XMLReader.h"

D3D12_SHADER_BYTECODE C2DShader::CreateVertexShader(ID3DBlob** ppd3dShaderBlob)
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSParticle", "vs_5_1", ppd3dShaderBlob));
}

D3D12_SHADER_BYTECODE C2DShader::CreatePixelShader(ID3DBlob** ppd3dShaderBlob)
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSParticle", "ps_5_1", ppd3dShaderBlob));
}

D3D12_BLEND_DESC C2DShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

D3D12_INPUT_LAYOUT_DESC C2DShader::CreateInputLayout()
{
	UINT nInputElementDescs = 3;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

void C2DShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255);
	int i = 0;
	for (CGameObject* object : object_list) {
		CB_GAMEOBJECT_INFO* pbMappedcbGameObject = (CB_GAMEOBJECT_INFO*)((UINT8*)m_pcbMappedGameObjects + (i * ncbElementBytes));
		XMStoreFloat4x4(&pbMappedcbGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&object->m_xmf4x4World)));
		++i;
		if (i >= max_object) break;
	}
}

C2DShader::C2DShader(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig)
{
	CreateShader(device, root_sig);
	BuildObjects(device, cmdlist);
}

C2DShader::~C2DShader()
{

}
#include "Configuration.h"
void C2DShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	max_object = MAX_PARTICLE;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, max_object, MAX_PARTICLE_TYPE);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, max_object, m_pd3dcbGameObjects, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));
}

CTexture* C2DShader::GetTexture(const wstring& tex_file_name)
{
	auto found = texture_map.find(tex_file_name);
	if (found != texture_map.end()) return (*found).second;
	CTexture* texture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	texture->LoadTextureFromDDSFile(DEVICEMANAGER.pd3dDevice, DEVICEMANAGER.pd3dCommandList, tex_file_name.c_str(), RESOURCE_TEXTURE2D, 0);
	AddTexture(DEVICEMANAGER.pd3dDevice, texture);
	texture_map.emplace(make_pair(tex_file_name, texture));
	return texture;
}

void C2DShader::AddTexture(ID3D12Device* device, CTexture* tex)
{
	CreateShaderResourceViews(device, tex, 0, (UINT)ROOT_PARAMATER_INDEX::TEXTURE);
}

void C2DShader::AddObject()
{

}

void C2DShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CTexturedShader::Render(pd3dCommandList, pCamera);
	/*for (CGameObject* object : object_list) {
		object->Render(pd3dCommandList, pCamera);
	}*/
	int i = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	for (CGameObject* object : object_list) {
		handle.ptr = m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * i);
		object->Render(pd3dCommandList, handle);
		++i;
		if (i >= max_object) break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//Sample

void C2DShaderSample::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	max_object = MAX_PARTICLE;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, max_object, MAX_PARTICLE_TYPE);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, max_object, m_pd3dcbGameObjects, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Resources/Particle/Flash.png", RESOURCE_TEXTURE2D, 0);
	AddTexture(pd3dDevice, pTexture);

	CMaterial* Material = new CMaterial();
	Material->SetTexture(pTexture);

	C2DObject* object = new C2DObject();
	object->SetMaterial(Material);
	object->SetPosition(50500.0f, CConfiguration::bottom + 200.0f, 14000.0f);
	object->SetCbvGPUDescriptorHandlePtr(m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * 0));
	object_list.push_back(object);
}

void C2DShaderSample::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CTexturedShader::Render(pd3dCommandList, pCamera);
	auto cameraPos = CGameFramework::GetInstance()->GetCamera()->GetPosition();
	object_list.sort([cameraPos](CGameObject* a, CGameObject* b) {
		float adis = Vector3::Length(Vector3::Subtract(a->GetPosition(), cameraPos));
		float bdis = Vector3::Length(Vector3::Subtract(b->GetPosition(), cameraPos));
		return adis > bdis;
	});
	for (CGameObject* object : object_list) {
		object->Render(pd3dCommandList, pCamera);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//UISystem

UISystem::UISystem(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist, ID3D12RootSignature* root_sig)
	: C2DShader()
{
	CreateShader(device, root_sig);
	BuildObjects(device, cmdlist);
	camera = new UICamera();
	camera->CreateShaderVariables(device, cmdlist);
	root_signagture = root_sig;
}

UISystem::~UISystem()
{
	if (camera) delete camera;
}

void UISystem::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	max_object = MAX_UI;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, max_object, MAX_UI_TYPE);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, max_object, m_pd3dcbGameObjects, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));

	CXMLReader::GetUISetting("Resources/UI/TestUI.xml", this);
}

void UISystem::AddUI(float width, float height, float x, float y, const wstring& image_file_name)
{
	CTexture* texture = GetTexture(image_file_name);
	CMaterial* material = new CMaterial();
	material->SetTexture(texture);
	CGameObject* object = new CUIObject(width, height, x, y, material);
	//object->SetCbvGPUDescriptorHandlePtr(m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * ));
	object_list.push_back(object);
}

void UISystem::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	pd3dCommandList->SetGraphicsRootSignature(root_signagture);

	camera->SetViewportsAndScissorRects(pd3dCommandList);
	camera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	C2DShader::Render(pd3dCommandList, camera);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//Particle

void ParticleSystem::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	XMFLOAT3 cameraPos;
	if(pCamera) auto cameraPos = pCamera->GetPosition();
	else {
#ifdef _DEBUG
		cout << "no Camera" << endl;
#endif
	}
	object_list.sort([cameraPos](CGameObject* a, CGameObject* b) {
		float adis = Vector3::Length(Vector3::Subtract(a->GetPosition(), cameraPos));
		float bdis = Vector3::Length(Vector3::Subtract(b->GetPosition(), cameraPos));
		return adis > bdis;
	});
	C2DShader::Render(pd3dCommandList, pCamera);
}

void ParticleSystem::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	C2DShader::BuildObjects(pd3dDevice, pd3dCommandList);
}