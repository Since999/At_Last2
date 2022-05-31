#pragma once
#include "Shader.h"
class CShadowShader :
    public CShader
{
public:
    CShadowShader() : CShader(){}
    ~CShadowShader() { }

    virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);

    virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
    virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
    //virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);
};

class CAnimationShadowShader :
    public CShadowShader
{
public:
    CAnimationShadowShader() : CShadowShader() {}
    ~CAnimationShadowShader() { }


    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
    //virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);
};

class CPlayerShadowShader :
    public CAnimationShadowShader
{
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
};