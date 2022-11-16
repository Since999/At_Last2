#pragma once
#include <functional>
#include "Object.h"

class UISystem;

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
private:
public:
    CUIObject(float width, float height, float x, float y, CMaterial* material = NULL);
    virtual void Animate(float fTimeElapsed);
    virtual bool CheckMouseCollision(float x, float y) { return false; }
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

class CProgressBar : public CUIObject {
private:
    float max_value;
    float value;
    float factor;
    atomic_int* value_ptr = NULL;
    XMFLOAT4X4 origin_mat;
public:
    CProgressBar(float width, float height, float x, float y, CMaterial* material, float max_value, atomic_int* value_ptr = NULL);
    void SetValue(float value);
    virtual void Animate(float fTimeElapsed);

    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL);
};

class CNumberUIComponent : public CUIObject {
private:
    static vector<CTexture*> textures;
    int value = 0;
public:
    static void SetTextures(vector<CTexture*> textures);
    CNumberUIComponent(float width, float height, float x, float y);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, const D3D12_GPU_DESCRIPTOR_HANDLE& desc_handle, CCamera* pCamera = NULL);

    void SetValue(int val) { value = val; }
};

class CNumberUI : public CGameObject {
private:
    float value = 0;
    vector<CNumberUIComponent*> component;
    int* value_ptr = NULL;
public:
    CNumberUI(float width, float height, float x, float y, int digit, UISystem& ui, int* value_ptr = NULL);

    virtual void Animate(float fTimeElapsed);

    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) {}
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}
    virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}
};

class CButtonUI : public CUIObject {
private:
    CCollisionRect collision_rect;
    function<void()> button_func;
    bool is_enable = true;
public:
    CButtonUI(float width, float height, float x, float y, function<void()> func, CMaterial* material = NULL);
    ~CButtonUI();

    virtual bool CheckMouseCollision(float x, float y);
    void Enable(bool is_enable = true) { this->is_enable = is_enable; }
    void Disable() { this->is_enable = false; }
};

class CPopupUI : public CUIObject {
private:
    float duration;
    float remain_time;
    
public:
    CPopupUI(float width, float height, float x, float y, float duration, CMaterial* material = NULL);

    virtual void Animate(float fTimeElapsed);
};

class CIPUI : public CGameObject {
private:
    const int digit = 4;
private:
    array<int, 4> address_value;
    vector<CNumberUI*> num_ui;
    string* value_ptr = NULL;
    int cur_index = 0;
    string cur_value;
    int num_cnt = 0;
public:
    CIPUI(float width, float height, float x, float y, UISystem& ui, string* value_ptr = NULL);

    virtual void Animate(float fTimeElapsed);

    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) {}
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}
    virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) {}

    void InputNumber(int num) {
        if (num_cnt >= 3) {
            cur_value.clear();
            cur_index += 1;
            num_cnt = 0;
        }
        if (cur_index >= 4) cur_index = 0;

        cur_value.push_back(num + '0');
        address_value[cur_index] = stoi( cur_value);
        num_cnt++;
    }

    void SetIP() {
        string address;
        for (int i : address_value) {
            address.append(to_string(i));
            address.push_back('.');
        }
        address.pop_back();
        IP_ADDRESS::SERVER_IP = address;
    }
};
