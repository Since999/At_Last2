#pragma once
#include "Object.h"
class C2DObject :
    public CGameObject
{
private:
    static CMesh* mesh;
public:
    C2DObject();
    //static vector<CVertex> Create2DMesh();
    virtual void Animate(float fTimeElapsed);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL);
};


class CUIObject : public C2DObject {
public:
    CUIObject(float width, float height, float x, float y, CMaterial* material);
    virtual void Animate(float fTimeElapsed);
};