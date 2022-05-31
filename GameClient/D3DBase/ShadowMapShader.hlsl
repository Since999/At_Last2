#include "common.hlsl"

float4 Shadow_VS(VS_TEXTURED_INPUT input) : SV_POSITION
{
	VS_TEXTURED_OUTPUT output;
	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	return output.position;
}

float4 ShadowAnimation_VS(VS_ANIMATED_INPUT input) : SV_POSITION
{
	VS_TEXTURED_OUTPUT output;

	float finalWeight = 1 - (input.weight[0] + input.weight[1] + input.weight[2]);


	float4x4 boneTransform = matBones[input.boneIdx[0]] * input.weight[0];
	boneTransform += matBones[input.boneIdx[1]] * input.weight[1];
	boneTransform += matBones[input.boneIdx[2]] * input.weight[2];
	boneTransform += matBones[input.boneIdx[3]] * finalWeight;

	output.position = mul(float4(input.position, 1.f), boneTransform);
	output.position = mul(output.position, AnimationObject);
	output.position = mul(output.position, gmtxView);
	output.position = mul(output.position, gmtxProjection);

	//output.uv = input.uv;

	return output.position;
}

float4 ShadowPlayer_VS(VS_ANIMATED_INPUT input) : SV_POSITION
{
	VS_TEXTURED_OUTPUT output;

	float finalWeight = 1 - (input.weight[0] + input.weight[1] + input.weight[2]);


	float4x4 boneTransform = PlayermatBones[input.boneIdx[0]] * input.weight[0];
	boneTransform += PlayermatBones[input.boneIdx[1]] * input.weight[1];
	boneTransform += PlayermatBones[input.boneIdx[2]] * input.weight[2];
	boneTransform += PlayermatBones[input.boneIdx[3]] * finalWeight;

	output.position = mul(float4(input.position, 1.f), boneTransform);
	output.position = mul(output.position, PlayerObject);
	output.position = mul(output.position, gmtxView);
	output.position = mul(output.position, gmtxProjection);

	//output.uv = input.uv;

	return output.position;
}