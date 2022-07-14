#include "CStaticObjectShader.h"
#include "ShadowShader.h"
#include "Network.h"

CStaticObjectShader* CStaticObjectShader::singleton = NULL;
 CStaticObjectShader* CStaticObjectShader::GetInstance()
 {
	 if (singleton) return singleton;
	 return NULL;
 }

 CStaticObjectShader* CStaticObjectShader::InitInstance(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
 {
	 if (singleton) return singleton;
	 singleton = new CStaticObjectShader();
	 singleton->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	 singleton->BuildObjects(pd3dDevice, pd3dCommandList, NULL);
	 return singleton;
 }

CStaticObjectShader::CStaticObjectShader()
{
}

CStaticObjectShader::~CStaticObjectShader()
{
}

void CStaticObjectShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	CTexturedShader::CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	shadow_shader = new CShadowShader();
	shadow_shader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
}


void CStaticObjectShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256ÀÇ ¹è¼ö
	m_pd3dcbGameObjects = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes * object_num, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

}

void CStaticObjectShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	
	/*UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255);
	for (int j = 0; j < m_nObjects; j++)
	{
		CB_GAMEOBJECT_INFO* pbMappedcbGameObject = (CB_GAMEOBJECT_INFO*)((UINT8*)m_pcbMappedGameObjects + (j * ncbElementBytes));
		XMStoreFloat4x4(&pbMappedcbGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_ppObjects[j]->m_xmf4x4World)));
	}*/
}

void CStaticObjectShader::ReleaseShaderVariables()
{
	if (m_pd3dcbGameObjects)
	{
		m_pd3dcbGameObjects->Unmap(0, NULL);
		m_pd3dcbGameObjects->Release();
	}

	CTexturedShader::ReleaseShaderVariables();
}


#include "OldModelLoader.h"
#include "TexturePool.h"
void CStaticObjectShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	vector<BarricadePos> barricade;
#ifdef ENABLE_NETWORK

	for (int i = 0; i < 42; ++i) {
		barricade.push_back(Network::one_barricade[i]);
	}for (int i = 0; i < 32; ++i) {
		barricade.push_back(Network::two_barricade[i]);
	}for (int i = 0; i < 30; ++i) {
		barricade.push_back(Network::three_barricade[i]);
	}for (int i = 0; i < 30; ++i) {
		barricade.push_back(Network::three_barricade2[i]);
	}for (int i = 0; i < 3; ++i) {
		barricade.push_back(Network::skill_barricade[i]);
	}
#else
	for (int i = 0; i < 10; ++i) {
		BarricadePos pos;
		pos.x = 50500.0f;
		pos.z = 14000.0f + 1000.0f * i;
		pos.dir = DIR::WIDTH;
		barricade.push_back(pos);
	}
	for (int i = 0; i < 10; ++i) {
		BarricadePos pos;
		pos.x = 50500.0f + 5000.0f * i;
		pos.z = 14000.0f;
		pos.dir = DIR::WIDTH;
		barricade.push_back(pos);
	}
#endif
	object_num = barricade.size() + MAX_ADDITIONAL_OBJECT;
	vector<CTexture*> textures;
	auto tex = CTexturePool::GetInstance()->GetTexture(L"no_texture.png");
	textures.push_back(tex);

	CreateCbvSrvDescriptorHeaps(pd3dDevice, object_num, 0);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateConstantBufferViews(pd3dDevice, object_num, m_pd3dcbGameObjects, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));

	vector<CMaterial*> materials;
	for (int i = 0; i < textures.size(); i++)
	{
		CMaterial* material = new CMaterial();
		material->SetTexture(textures[i]);
		materials.push_back(material);
	}

	OldModelLoader model_loader;
	model_loader.LoadModelWithTranslation(pd3dDevice, pd3dCommandList, std::string("Resources/Model/Barricada Concreto.fbx"), mesh);

	CStaticObject* static_object = NULL;

	float bottom = -500.0f;
	
	for(auto& bar_info : barricade){
		static_object = new CStaticObject(mesh.size());
		static_object->SetNumberOfMeshes(mesh.size());
		for (int j = 0; j < mesh.size(); ++j) {
			static_object->SetMesh(j, mesh[j]);
		}
		
		//pRotatingObject->SetMesh(0, pCubeMesh);
		static_object->SetMaterial(materials[0]);
		static_object->SetPosition(bar_info.x, bottom, bar_info.z);
		static_object->Scale(0.1f);
		if (bar_info.dir == DIR::WIDTH) {
			static_object->Rotate(0.0f, 0.0f, 0.0f);
		}
		else {
			static_object->Rotate(0.0f, 90.0f, 0.0f);
		}
		//pRotatingObject->SetRotationAxis(XMFLOAT3(0.0f, 1.0f, 0.0f));
		//pRotatingObject->SetRotationSpeed(10.0f * (i % 10));
		static_object->SetCbvGPUDescriptorHandlePtr(m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * cb_size));
		objects.push_back(static_object);
		cb_size++;
	}
	m_pd3dcbGameObjects->Map(0, NULL, (void**)&m_pcbMappedGameObjects);
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255);
	for (UINT j = 0; j < objects.size(); j++)
	{
		CB_GAMEOBJECT_INFO* pbMappedcbGameObject = (CB_GAMEOBJECT_INFO*)((UINT8*)m_pcbMappedGameObjects + (j * ncbElementBytes));
		XMStoreFloat4x4(&pbMappedcbGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&objects[j]->m_xmf4x4World)));
	}
	m_pd3dcbGameObjects->Unmap(0, NULL);
}

void CStaticObjectShader::ReleaseObjects()
{
	for (auto& object : objects) {
		delete object;
	}
}

void CStaticObjectShader::AnimateObjects(float fTimeElapsed)
{
	for (auto& object : objects) {
		object->Animate(fTimeElapsed);
	}
}

void CStaticObjectShader::ReleaseUploadBuffers()
{
	for (auto& object : objects) {
		object->ReleaseUploadBuffers();
	}
}

void CStaticObjectShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CTexturedShader::Render(pd3dCommandList,  pCamera);

	for (auto& object : objects) {
		object->Render(pd3dCommandList, m_pd3dCbvSrvDescriptorHeap, pCamera);
	}
}

void CStaticObjectShader::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList) {
	shadow_shader->Render(pd3dCommandList, NULL);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	for (auto& object : objects) {
		object->ShadowMapRender(pd3dCommandList, NULL);
	}
}

void CStaticObjectShader::AddBarricade(const BarricadePos& barricade)
{
	CStaticObject* static_object = NULL;

	auto tex = CTexturePool::GetInstance()->GetTexture(L"no_texture.png");

	CMaterial* material = new CMaterial();
	material->SetTexture(tex);

	float bottom = -500.0f;
	static_object = new CStaticObject(mesh.size());
	static_object->SetNumberOfMeshes(mesh.size());
	for (int j = 0; j < mesh.size(); ++j) {
		static_object->SetMesh(j, mesh[j]);
	}

	//pRotatingObject->SetMesh(0, pCubeMesh);
	static_object->SetMaterial(material);
	static_object->SetPosition(barricade.x, bottom, barricade.z);
	static_object->Scale(0.1f);

	if (barricade.dir == DIR::WIDTH) {
		static_object->Rotate(0.0f, 0.0f, 0.0f);
	}
	else {
		static_object->Rotate(0.0f, 90.0f, 0.0f);
	}

	static_object->SetCbvGPUDescriptorHandlePtr(m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * cb_size));
	objects.push_back(static_object);
	cb_size++;

	m_pd3dcbGameObjects->Map(0, NULL, (void**)&m_pcbMappedGameObjects);
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255);
	for (int j = 0; j < objects.size(); j++)
	{
		CB_GAMEOBJECT_INFO* pbMappedcbGameObject = (CB_GAMEOBJECT_INFO*)((UINT8*)m_pcbMappedGameObjects + (j * ncbElementBytes));
		XMStoreFloat4x4(&pbMappedcbGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&objects[j]->m_xmf4x4World)));
	}
	m_pd3dcbGameObjects->Unmap(0, NULL);
}