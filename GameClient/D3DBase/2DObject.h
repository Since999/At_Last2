#pragma once
#include "Object.h"
class C2DObject :
    public CGameObject
{
protected:
    static int root_par_index;
    static CMesh* mesh;
    XMFLOAT2 size;
    float transparent = 1.f;
public:
    C2DObject();
    //static vector<CVertex> Create2DMesh();
    virtual void Animate(float fTimeElapsed);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL);
    virtual float GetTransparent() const { return transparent; }
};


class CUIObject : public C2DObject {
public:
    CUIObject(float width, float height, float x, float y, CMaterial* material);
    virtual void Animate(float fTimeElapsed);
};

class CParticleObject : public C2DObject {
protected:
    float duration;
    float cur_time = 0.0f;
    vector<CMaterial*>* materials;

public:
    CParticleObject(float duration, const XMFLOAT2& size, const XMFLOAT3& position, vector<CMaterial*>* materials);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL);
    virtual void Animate(float fTimeElapsed);
};

class CTrail : public CParticleObject {
private:
    unsigned int index;
public:
    CTrail(float duration, const XMFLOAT2& size, const XMFLOAT3& position, vector<CMaterial*>* materials);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL);
    virtual void Animate(float fTimeElapsed);
};

class CParticleBuilder {
private:
    float duration;
    vector<CMaterial*>* materials;
    XMFLOAT2 size;

public:
    CParticleBuilder(float duration, const XMFLOAT2& size, vector<CMaterial*>* materials) :
        duration(duration), size(size), materials(materials) {}
    ~CParticleBuilder() {
    }
    CParticleObject* Build(const XMFLOAT3& position) {
        return new CParticleObject(duration, size, position, materials);
    }
    CParticleObject* BuildAsTrail(const XMFLOAT3& position) {
        return new CTrail(duration, size, position, materials);
    }
};