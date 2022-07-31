#pragma once
#include "Object.h"
class CBullet :
    public CGameObject
{
private:
    XMFLOAT3 direction;
    XMFLOAT3 initial_position;
    float speed;
    float time = 0.0f;
public:
    virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList){}
    virtual void ReleaseShaderVariables(){}
    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList){}

    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12DescriptorHeap* desc_heap, CCamera* pCamera = NULL) {}
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL) {}
    virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}

    CBullet(const XMFLOAT3& position, const XMFLOAT3& direction, float speed = 25000);
    virtual void Animate(float fTimeElapsed);

   
};

