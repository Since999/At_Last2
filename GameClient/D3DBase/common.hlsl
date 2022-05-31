#include "Lighting.hlsl"

cbuffer PlayerInfo : register(b0)
{
	matrix		PlayerObject : packoffset(c0);
	matrix		PlayermatBones[96] : packoffset (c4);
};

cbuffer cbCameraInfo : register(b1)
{
	matrix		gmtxView : packoffset(c0);
	matrix		gmtxProjection : packoffset(c4);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix		gmtxGameObject : packoffset(c0);
};

cbuffer AnimationObjectInfo : register(b3)
{
	matrix		AnimationObject : packoffset(c0);
	matrix		matBones[96] : packoffset (c4);
};

cbuffer Light : register(b4)
{
    matrix gShadowTransform;
    matrix gShadowView;
    matrix gShadowProjection;
    float3 gLightPos;
    float3 gLightDir;
}

Texture2D gtxtTexture : register(t0);
SamplerState gSamplerState : register(s0);

Texture2D gShadowMap : register(t1);
SamplerComparisonState gsamShadow : register(s1);


static matrix T = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
};

struct VS_ANIMATED_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float4 weight : BONEWEIGHT;
	uint4 boneIdx: BONEINDEX;
};

struct VS_TEXTURED_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT
{
	//float4 positionH    : SV_POSITION;
	//float4 positionW    : POSITION0;
	float4 position    : SV_POSITION;
    float4 positionW : POSITION0;
    float4 shadow_pos : POSITION1;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    float3 pos = shadowPosH.xyz / shadowPosH.w;

    // Depth in NDC space.
    float depth = pos.z + 0.005;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            pos.xy + offsets[i], depth).r;
    }
    float result = percentLit / 9.0f;
    return result;
}