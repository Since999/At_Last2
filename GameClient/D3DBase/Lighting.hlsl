#include "Header.hlsli"

float3 Calc_light(float4 normal, float4 shadow_pos, float4 light_dir, float3 light_color)
{
	// ambient light
	float ambientLight = 0.3f;
	float3 ambient = float3(ambientLight, ambientLight, ambientLight);

	//diffuse light
	float diffuseLight = max(dot(normal, light_dir), 0.0);
	//diffuseLight = diffuseLight * diffuseLight;

	float3 diffuse = CalcShadowFactor(shadow_pos) * diffuseLight * light_color;
	//float3 diffuse = diffuseLight * lightColor;

	//int shininess = 128;
	//vec3 viewDir = normalize(viewPos - vec3(FragPos));
	//vec3 reflectDir = reflect(-lightDir, normalVector);
	//float specularLight = max(dot(viewDir, reflectDir), 0.0);
	//specularLight = pow(specularLight, shininess);
	//vec3 specular = specularLight * lightColor;
	return  ambient + diffuse;
}