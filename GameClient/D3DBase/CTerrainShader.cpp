#include "CTerrainShader.h"
#include "ShadowShader.h"

CTerrainShader::CTerrainShader()
{
}

CTerrainShader::~CTerrainShader()
{
	if (shadow_shader) delete shadow_shader;
}

void CTerrainShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	CTexturedShader::CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	shadow_shader = new CShadowShader();
	shadow_shader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
}

void CTerrainShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256ÀÇ ¹è¼ö
	m_pd3dcbGameObjects = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes * m_nObjects, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbGameObjects->Map(0, NULL, (void**)&m_pcbMappedGameObjects);
}

void CTerrainShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255);
	CB_GAMEOBJECT_INFO* pbMappedcbGameObject = (CB_GAMEOBJECT_INFO*)((UINT8*)m_pcbMappedGameObjects + (0 * ncbElementBytes));
	//XMStoreFloat4x4(&pbMappedcbGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_ppObjects[0]->m_xmf4x4World)));
	XMStoreFloat4x4(&pbMappedcbGameObject->m_xmf4x4World, XMMatrixIdentity());
}

D3D12_INPUT_LAYOUT_DESC CTerrainShader::CreateInputLayout()
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

void CTerrainShader::ReleaseShaderVariables()
{
	if (m_pd3dcbGameObjects)
	{
		m_pd3dcbGameObjects->Unmap(0, NULL);
		m_pd3dcbGameObjects->Release();
	}

	CTexturedShader::ReleaseShaderVariables();
}

#include "OldModelLoader.h"
#include "ModelLoader.h"
#include "ShadowShader.h"
#include "TexturePool.h"
void CTerrainShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	OldModelLoader model_loader;
	std::vector<CMesh*> tmp;
	model_loader.LoadModelWithTranslation(pd3dDevice, pd3dCommandList, std::string("Resources/Model/TEST5.fbx"), tmp);
	std::vector<CTexture*> textures;
	auto texture_pool = CTexturePool::GetInstance();
	for (int i = 0; i < tmp.size(); ++i) {
		std::wstring dir{ tmp[i]->GetTextureDir() };
		if (dir.empty()) {
			textures.push_back(NULL);
			continue;
		}
		textures.push_back(texture_pool->GetTexture(dir));
	}
	m_nObjects = 1;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, m_nObjects, 0);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, m_nObjects, m_pd3dcbGameObjects, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));

	m_nObjects = tmp.size();


	m_ppObjects = new CGameObject * [m_nObjects];
	CGameObject* static_object = NULL;
	for (int i = 0; i < m_nObjects; ++i) {
		static_object = new CStaticObject();
		static_object->SetNumberOfMeshes(tmp.size());
		static_object->SetMesh(0, tmp[i]);
		CMaterial* m = new CMaterial();
		m->SetTexture(textures[i]);
		static_object->SetMaterial(m);
		static_object->SetCbvGPUDescriptorHandlePtr(m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * 0));
		m_ppObjects[i] = static_object;
	}
}

void CTerrainShader::ReleaseObjects()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) delete m_ppObjects[j];
		delete[] m_ppObjects;
	}
}

void CTerrainShader::AnimateObjects(float fTimeElapsed)
{
	/*for (int j = 0; j < m_nObjects; j++)
	{
		m_ppObjects[j]->Animate(fTimeElapsed);
	}*/
}

void CTerrainShader::ReleaseUploadBuffers()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) m_ppObjects[j]->ReleaseUploadBuffers();
	}
}

void CTerrainShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CTexturedShader::Render(pd3dCommandList, pCamera);

	for (int j = 0; j < m_nObjects; j++)
	{
		if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, m_pd3dCbvSrvDescriptorHeap, pCamera);
	}
}

void CTerrainShader::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList) {
	shadow_shader->Render(pd3dCommandList, NULL);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);
	for (int j = 0; j < m_nObjects; j++)
	{
		if (m_ppObjects[j]) m_ppObjects[j]->ShadowMapRender(pd3dCommandList, NULL);
	}
}