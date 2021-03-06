#include "2DObject.h"
#include "Shader.h"
#include "GameFramework.h"
#include "2DShader.h"

CMesh* C2DObject::mesh = NULL;
int C2DObject::root_par_index = (int)ROOT_PARAMATER_INDEX::GAMEOBJECT;

C2DObject::C2DObject()
{
	if (!mesh) mesh = C2DMesh::GetInstance();
	size = { 1000.f, 1000.f };
}

void C2DObject::Animate(float fTimeElapsed)
{
	auto camera = CGameFramework::GetInstance()->GetCamera();
	auto target = CGameFramework::GetInstance()->GetCamera()->GetPosition();
	XMFLOAT3 up = CGameFramework::GetInstance()->GetCamera()->GetUpVector();
	/*XMFLOAT3 look = Vector3::Normalize(Vector3::Subtract(target, GetPosition()));
	XMFLOAT3 right = Vector3::CrossProduct(up, look, true);*/
	XMFLOAT3 look = camera->GetLookVector();
	XMFLOAT3 right = camera->GetRightVector();
	m_xmf4x4World._11 = right.x; m_xmf4x4World._12 = right.y; m_xmf4x4World._13 = right.z;
	m_xmf4x4World._21 = up.x; m_xmf4x4World._22 = up.y; m_xmf4x4World._23 = up.z;
	m_xmf4x4World._31 = look.x; m_xmf4x4World._32 = look.y; m_xmf4x4World._33 = look.z;
	
	//Rotate(fTimeElapsed * 10.0f, fTimeElapsed * 5.0f, fTimeElapsed * 2.0f);
	Scale(size.x, size.y, 1.0f);
}

void C2DObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (m_pMaterial && m_pMaterial->m_pTexture) {
		m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, m_d3dCbvGPUDescriptorHandle);
	
	if (mesh) mesh->Render(pd3dCommandList);
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}


void C2DObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (m_pMaterial && m_pMaterial->m_pTexture) {
		m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, desc_handle);

	if (mesh) mesh->Render(pd3dCommandList);
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}

CUIObject::CUIObject(float width, float height, float x, float y, CMaterial* material)
{
	Scale(width, height, 0);
	SetPosition(x, y, 0);
	SetMaterial(material);
}

void CUIObject::Animate(float fTimeElapsed)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//	CParticleObject
CParticleObject::CParticleObject(float duration, const XMFLOAT2& size, 
	const XMFLOAT3& position, vector<CMaterial*>* materials)
	: duration(duration), materials(materials)
{
	this->size = size;
	SetPosition(position);
}


void CParticleObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, 
	const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera)
{
	OnPrepareRender();
	float factor = cur_time / duration;	// 0~1
	int index = (int)(materials->size() * factor);
	if (index >= materials->size()) {
#ifdef _DEBUG
		cout << "warning(particle render): index over size " << endl;
#endif
		return;
	}
	CMaterial* mat = (*materials)[index];
	if (mat)
	{
		if (mat->m_pShader)
		{
			mat->m_pShader->Render(pd3dCommandList, pCamera);
			mat->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (mat && mat->m_pTexture) {
		mat->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, desc_handle);

	if (mesh) mesh->Render(pd3dCommandList);
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}

void CParticleObject::Animate(float fTimeElapsed)
{
	cur_time += fTimeElapsed;
	if (cur_time >= duration) {
		ParticleSystem::GetInstance()->RemoveObject(this);
		//???? remove this
		return;
	}
	C2DObject::Animate(fTimeElapsed);
}

CTrail::CTrail(float duration, const XMFLOAT2& size, const XMFLOAT3& position, vector<CMaterial*>* materials)
	:CParticleObject(duration, size, position, materials)
{
	index = rand() % materials->size();
}

void CTrail::Render(ID3D12GraphicsCommandList* pd3dCommandList,
	const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera)
{
	OnPrepareRender();
	CMaterial* mat = (*materials)[index];
	if (mat)
	{
		if (mat->m_pShader)
		{
			mat->m_pShader->Render(pd3dCommandList, pCamera);
			mat->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (mat && mat->m_pTexture) {
		mat->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, desc_handle);

	if (mesh) mesh->Render(pd3dCommandList);
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}

void CTrail::Animate(float fTimeElapsed)
{
	CParticleObject::Animate(fTimeElapsed);
	transparent = cur_time / duration;
	transparent = 1.f - clamp(transparent, 0.f, 1.f);
	transparent *= transparent;
}

CProgressBar::CProgressBar(float width, float height, float x, float y, CMaterial* material, float max_value, atomic_int* value_ptr)
	:CUIObject(width, height, x, y, material), max_value(max_value), value_ptr(value_ptr)
{
	origin_mat = m_xmf4x4World;
	factor = 1;
	SetMesh(0, C2DMeshLeftAnchor::GetInstance());
}

void CProgressBar::SetValue(float value)
{
	this->value = value;
	this->factor = value / max_value;
}

float smooth_time = 0.5f;

void CProgressBar::Animate(float fTimeElapsed)
{
	CUIObject::Animate(fTimeElapsed);
	if (value_ptr) {
		value = (*value_ptr);
	}
	float real_factor = value / max_value;
	float m = real_factor - factor;
	factor = factor + m * (fTimeElapsed / smooth_time);
	XMMATRIX mtxScale = XMMatrixScaling(factor, 1, 1);
	m_xmf4x4World = Matrix4x4::Multiply(mtxScale, origin_mat);
}

void CProgressBar::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (m_pMaterial && m_pMaterial->m_pTexture) {
		m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, m_d3dCbvGPUDescriptorHandle);

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList);
		}
	}
#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}

void CProgressBar::Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}

	if (m_pMaterial && m_pMaterial->m_pTexture) {
		m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, desc_handle);

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList);
		}
	}

#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}


vector<CTexture*> CNumberUIComponent::textures;

void CNumberUIComponent::SetTextures(vector<CTexture*> tex)
{
	textures = tex;
}

CNumberUIComponent::CNumberUIComponent(float width, float height, float x, float y) 
	: CUIObject(width, height, x, y)
{

}

void CNumberUIComponent::Render(ID3D12GraphicsCommandList* pd3dCommandList, 
	const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera)
{
	OnPrepareRender();

	/*if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
	}*/

	//if (m_pMaterial && m_pMaterial->m_pTexture) {
	//	m_pMaterial->m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);
	//}
	if (value < 0) return;
	if (value < textures.size()) {
		textures[value]->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}
#ifdef _DEBUG
	else {
		cout << "error (NumberUI): value is over size" << endl;
		cout << "value: " << value << ", texture: " << textures.size() << endl;
		return;
	}
#endif

	pd3dCommandList->SetGraphicsRootDescriptorTable(root_par_index, desc_handle);

	if (mesh) mesh->Render(pd3dCommandList);

#ifdef _DEBUG
	else {
		cout << "Error(2DObject): no mesh" << endl;
	}
#endif
}

CNumberUI::CNumberUI(float width, float height, float x, float y, int digit, UISystem& ui, int* value_ptr)
	:CGameObject(0), value_ptr(value_ptr)
{
	float w = width / digit;

	float right = x + width / 2 - w / 2;
	float Cx;
	
	for (int i = 0; i < digit; ++i) {
		Cx = right - w * i;
		component.push_back(new CNumberUIComponent(w, height, Cx, y));
		ui.AddObject(component[i]);
	}
}

void CNumberUI::Animate(float fTimeElapsed)
{
	if (value_ptr) {
		value = *value_ptr;
	}
	int i = 0;
	for ( ; value / pow(10, i) >= 1 && i < component.size(); ++i) {
		int num = value / pow(10, i);
		num = num % 10;
		component[i]->SetValue(num);
	}
	for (; i < component.size(); ++i) {
		component[i]->SetValue(0);
	}
}
CButtonUI::CButtonUI(float width, float height, float x, float y, function<void()> func, CMaterial* material)
	: CUIObject(width, height, x, y, material), button_func(func)
{
	float half_wid = width / 2;
	float half_height = height / 2;
	collision_rect.SetRect(x - half_wid, y + half_height, x + half_wid, y - half_height);
}

CButtonUI::~CButtonUI()
{

}

bool CButtonUI::CheckMouseCollision(float x, float y)
{
	if (collision_rect.CheckCollision(x, y)) {
		button_func();
		return true;
	}
	return false;
}
