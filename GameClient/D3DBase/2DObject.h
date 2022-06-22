#pragma once
#include "Object.h"
class C2DObject :
    public CGameObject
{
public:
    C2DObject(ID3D12Device* device, ID3D12GraphicsCommandList* cmdlist);
    //static vector<CVertex> Create2DMesh();
    virtual void Animate(float fTimeElapsed);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
};

