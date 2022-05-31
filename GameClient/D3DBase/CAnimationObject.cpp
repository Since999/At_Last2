#include "CAnimationObject.h"
#include "SkinModel.h"
#include "Shader.h"
#include "ModelLoader.h"
#include "AnimationShader.h"
#include "TexturePool.h"
#include "ShadowShader.h"
#include "StateMachine.h"
#include "ModelManager.h"

//CAnimationObject::CAnimationObject() :CGameObject(0)
//{
//	
//}


void CAnimationObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{

	UINT ncbElementBytes = ((sizeof(CB_ANIMATION_OBJECT_INFO) + 255) & ~255); //256ÀÇ ¹è¼ö
	ani_info_resource = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	//m_pd3dcbPlayer->Map(0, NULL, (void**)&m_pcbMappedPlayer);

}

void CAnimationObject::ReleaseShaderVariables()
{

	if (ani_info_resource)
	{
		ani_info_resource->Unmap(0, NULL);
		ani_info_resource->Release();
	}
}

void CAnimationObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!enabled) return;
	ani_info_resource->Map(0, NULL, (void**)&mapped_ani_info);

	XMStoreFloat4x4(&mapped_ani_info->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	
	auto& matBoneList = GetBoneMat();
	
	for (UINT i = 0; i < matBoneList.size(); i++)
		mapped_ani_info->gBoneTransforms[i] = matBoneList[i];

	ani_info_resource->Unmap(0, NULL);
}

void CAnimationObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList, CB_ANIMATION_OBJECT_INFO* cb)
{
	if (!enabled) return;

	XMStoreFloat4x4(&cb->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));

	auto& matBoneList = GetBoneMat();

	for (UINT i = 0; i < matBoneList.size(); i++)
		cb->gBoneTransforms[i] = matBoneList[i];
}

void CAnimationObject::SetConstBuffer(ID3D12GraphicsCommandList* pd3dCommandList)
{
	auto d3dGpuVirtualAddress = ani_info_resource->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(0, d3dGpuVirtualAddress);
}

CAnimationObject::CAnimationObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
	const string& model, const wchar_t* texture) : CPlayer(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, 0)
{
	this->model = ModelLoader::LoadModel(model.c_str());
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	this->model->PlayAni(0);
	
	CTexture* tex = CTexturePool::GetInstance()->GetTexture(texture);
	
	//CGameObject::Scale(0.01f);

	CMaterial* ppMaterials = new CMaterial();
	ppMaterials->SetTexture(tex);
	SetMaterial(ppMaterials);

	CAnimationShader* pShader = new CAnimationShader();


	pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 0);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pShader->CreateConstantBufferViews(pd3dDevice, 1, ani_info_resource, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));

	SetCbvGPUDescriptorHandle(pShader->GetGPUCbvDescriptorStartHandle());

	SetShader(pShader);

	CAnimationShadowShader* shadow_shader = new CAnimationShadowShader();
	shadow_shader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	SetShadowShader(shadow_shader);
}
CAnimationObject::CAnimationObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
	SkinModel* model) : CPlayer(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, 0) {
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	this->model = model;

	CAnimationShader* pShader = new CAnimationShader();


	pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 0);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pShader->CreateConstantBufferViews(pd3dDevice, 1, ani_info_resource, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));
	//pShader->CreateShaderResourceViews(pd3dDevice, model->GetTexture(), 0, 3);

	SetCbvGPUDescriptorHandle(pShader->GetGPUCbvDescriptorStartHandle());

	SetShader(pShader);

	CAnimationShadowShader* shadow_shader = new CAnimationShadowShader();
	shadow_shader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	SetShadowShader(shadow_shader);
}

CAnimationObject::CAnimationObject() :CPlayer(NULL, NULL, NULL, NULL, 0)
{
}

CAnimationObject::~CAnimationObject()
{
	if (state_machine) delete state_machine;
}

void CAnimationObject::Animate(float fTimeElapsed)
{
	if (!enabled) return;
	//animation_time += fTimeElapsed;

	//if (state_machine) state_machine->Update(fTimeElapsed);
	//else model->UpdateNodeTM(animation_time, fTimeElapsed);

	auto curr_pos = GetPosition();
	speed = Vector3::Length(Vector3::Subtract(pre_location, curr_pos)) / fTimeElapsed;
	pre_location = curr_pos;
}

void CAnimationObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!enabled) return;

	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
		if (m_pMaterial->m_pTexture)
		{
			m_pMaterial->m_pTexture->UpdateShaderVariables(pd3dCommandList);
		}
	}

	auto d3dGpuVirtualAddress = ani_info_resource->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(0, d3dGpuVirtualAddress);

	model->Render(pd3dCommandList);
}

void CAnimationObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12DescriptorHeap* desc_heap, UINT64 desc_handle_ptr, CCamera* pCamera)
{
	if (!enabled) return;

	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pTexture)
		{
			m_pMaterial->m_pTexture->UpdateShaderVariables(pd3dCommandList);
		}
	}
	
	pd3dCommandList->SetDescriptorHeaps(1, &desc_heap);
	D3D12_GPU_DESCRIPTOR_HANDLE desc_handle;
	desc_handle.ptr = desc_handle_ptr;
	pd3dCommandList->SetGraphicsRootDescriptorTable((int)ROOT_PARAMATER_INDEX::ANIMATION, desc_handle);

	model->Render(pd3dCommandList);
}

void CAnimationObject::ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList, UINT64 desc_handle_ptr, CCamera* pCamera)
{
	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShadowShader)
		{
			m_pMaterial->m_pShader->OnPrepareShadowRender(pd3dCommandList);
			m_pMaterial->m_pShadowShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShadowShader->UpdateShaderVariables(pd3dCommandList);
		}
	}
	D3D12_GPU_DESCRIPTOR_HANDLE desc_handle;
	desc_handle.ptr = desc_handle_ptr;
	pd3dCommandList->SetGraphicsRootDescriptorTable((int)ROOT_PARAMATER_INDEX::ANIMATION, desc_handle);

	model->ShadowRender(pd3dCommandList);
}

//-------------------------------------------------------------------------------------

CZombie::CZombie(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
	SkinModel* model) : CAnimationObject(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL) {
	if (model == NULL) {
		SkinModel* nmodel = ModelManager::GetModel("Z1_Nomal_.fbx", L"Zombie.tga");
		nmodel->GetAnimationList()[4].SetRepeat(false);
		/*SkinModel* nmodel = ModelLoader::LoadModel("Resources/Model/Z1_Nomal_.fbx");

		CTexture* tex = CTexturePool::GetInstance()->GetTexture(L"Resources/Texture/Zombie.tga");
		nmodel->SetTexture(tex);*/

		//nmodel->PlayAni(0);
		this->model = nmodel;
	}
	else {
		this->model = model;
	}
	state_machine = new CZombieStateMachine(this);
}

CZombie::CZombie() : CAnimationObject()
{
	if (model == NULL) {
		SkinModel* nmodel = ModelManager::GetModel("Z1_Nomal_.fbx", L"Zombie.png");

		/*SkinModel* nmodel = ModelLoader::LoadModel("Resources/Model/Z1_Nomal_.fbx");

		CTexture* tex = CTexturePool::GetInstance()->GetTexture(L"Resources/Texture/Zombie.tga");
		nmodel->SetTexture(tex);*/

		//nmodel->PlayAni(0);
		this->model = nmodel;
	}
	else {
		this->model = model;
	}
	state_machine = new CZombieStateMachine(this);
}

void CZombie::Animate(float fTimeElapsed) 
{
#ifdef ENABLE_NETWORK
	if (!zombie) return;
	if (!zombie->view.load()) return;

	SetPosition((zombie->x - 550.0f) * (-100.0f), GetPosition().y, (zombie->z - 210.0f) * (-100.0f));

	SetYaw(XMConvertToRadians(zombie->angle.load()));
#endif
	CAnimationObject::Animate(fTimeElapsed);
}

void CZombie::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
#ifdef ENABLE_NETWORK
	if (!zombie->view.load()) return;
#endif
	CAnimationObject::Render(pd3dCommandList, pCamera);
}

void CAnimationObject::ChangeAni()
{
	state_machine->ChangeAni();
}