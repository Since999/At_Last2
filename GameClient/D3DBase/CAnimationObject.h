#pragma once
//#include "Object.h"
#include "Player.h"

class SkinModel;
class CCamera;
class CStateMachine;

struct CB_ANIMATION_OBJECT_INFO
{
    XMFLOAT4X4					m_xmf4x4World;
    XMMATRIX					gBoneTransforms[96];
};

class CAnimationObject :
    public CPlayer
{
private:
    //SkinModel* model = NULL;
    CB_ANIMATION_OBJECT_INFO* mapped_ani_info;
    ID3D12Resource* ani_info_resource;
    float animation_time = 0.f;
    
    bool enabled = true;
    
protected:

    XMFLOAT3 pre_location;
    float speed;
    float move_angle;
public:
    CAnimationObject();
    CAnimationObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
        const string& model, const wchar_t* texture);
    CAnimationObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
        SkinModel* model);
    ~CAnimationObject();
    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList, CB_ANIMATION_OBJECT_INFO* cb);
protected:
    virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
    virtual void ReleaseShaderVariables();
    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

public:
    virtual void SetConstBuffer(ID3D12GraphicsCommandList* pd3dCommandList);

    virtual void Animate(float fTimeElapsed);

    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12DescriptorHeap* desc_heap, UINT64 desc_handle_ptr, CCamera* pCamera = NULL);
    virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList, UINT64 desc_handle_ptr, CCamera* pCamera = NULL);
    float GetSpeed() const { return speed; }

    void ChangeAni();
};

class CZombie : public CAnimationObject {
    Zombie* zombie = NULL;
public:
    CZombie(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
        const string& model, const wchar_t* texture);
    CZombie(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
        SkinModel* model);
    CZombie(const string& model_name = "Z1.fbx", const wstring& tex_name = L"Zombie.png");
    //~CZombie();

public:
    void SetZombie(Zombie* zom) { zombie = zom; }
    Zombie* GetZombie() { return zombie; }

    virtual void Animate(float fTimeElapsed);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
};