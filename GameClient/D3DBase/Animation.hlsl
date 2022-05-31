#include "common.hlsl"

VS_TEXTURED_OUTPUT VSAnimated(VS_ANIMATED_INPUT input)
{
	VS_TEXTURED_OUTPUT output;

	float finalWeight = 1 - (input.weight[0] + input.weight[1] + input.weight[2]);

	float4x4 boneTransform = matBones[input.boneIdx[0]] * input.weight[0];
	boneTransform += matBones[input.boneIdx[1]] * input.weight[1];
	boneTransform += matBones[input.boneIdx[2]] * input.weight[2];
	boneTransform += matBones[input.boneIdx[3]] * finalWeight;

	matrix world_mat = mul(boneTransform, AnimationObject);
	output.positionW = mul(float4(input.position, 1.f), world_mat);
	//output.positionW = mul(output.positionW, PlayerObject);
	output.position = mul(output.positionW, gmtxView);
	output.position = mul(output.position, gmtxProjection);

	output.shadow_pos = mul(output.positionW, mul(mul(gShadowView, gShadowProjection), T));
	output.uv = input.uv;
	output.normal = mul(float4(input.normal, 0.0f), world_mat);

	return output;
}

float4 PSAnimated(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTexture.Sample(gSamplerState, input.uv);
	//if (cColor.w < 0.1f) discard;
	float4 result = float4(Calc_light(input.normal, input.shadow_pos, float4(-gLightDir, 0.0f), float3(1.0f, 1.0f, 1.0f)), 1.0f) * cColor;
	return result;
}
