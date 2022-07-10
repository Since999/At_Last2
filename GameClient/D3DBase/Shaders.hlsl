#include "common.hlsl"

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input)
{
	VS_TEXTURED_OUTPUT output;
	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject);
	output.position = mul(mul(output.positionW, gmtxView), gmtxProjection);
	output.shadow_pos = mul(output.positionW, mul(mul(gShadowView, gShadowProjection), T));
	
	output.uv = input.uv;
	output.normal = mul(float4(input.normal, 0.0f), gmtxGameObject);
	return(output);
}

float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTexture.Sample(gSamplerState, input.uv);
	if (cColor.w < 0.1f) discard;
	
	float4 result = float4(Calc_light(input.normal, input.shadow_pos, float4(-gLightDir, 0.0f), float3(0.8f, 0.8f, 0.8f)), 1.0f) * cColor;
	//vec4 result = vec4((ambient + diffuse + specular), 1.0f) * texture(texture1, TexCoord);
	//FragColor = result;

	return(result);
}

struct PARTICLE_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PARTICLE_OUTPUT
{
	//float4 positionH    : SV_POSITION;
	//float4 positionW    : POSITION0;
	float4 position    : SV_POSITION;
	float2 uv : TEXCOORD;
};

PARTICLE_OUTPUT VSParticle(PARTICLE_INPUT input)
{
	PARTICLE_OUTPUT output;
	//float4 rotation = float4(m_xmf4x4View._13, m_xmf4x4View._23, m_xmf4x4View._33, 1.0f);
	//float4 toCamera = mul(float4(input.position, 1.0f), );
	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.uv = input.uv;
	return(output);
}

float4 PSParticle(PARTICLE_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTexture.Sample(gSamplerState, input.uv);
	cColor.a = cColor.a * Transparent;
	float4 result = cColor;

	return(result);
}
