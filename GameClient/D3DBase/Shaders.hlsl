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

VS_TEXTURED_OUTPUT VSParticle(VS_TEXTURED_INPUT input)
{
	VS_TEXTURED_OUTPUT output;
	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.uv = input.uv;
	return(output);
}

float4 PSParticle(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTexture.Sample(gSamplerState, input.uv);

	float4 result = cColor;

	return(result);
}
