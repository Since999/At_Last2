#include "common.hlsl"




void light(float3 pos, float4 normal, float4 lightDir, float3 lightColor)
{
	float4 lightDir = float4(normalize(float3(0.25f, 0.5f, 0.25f)), 1.0f);
	float3 lightColor = { 0.7f, 0.7f, 0.7f };

	// ambient light
	float ambientLight = 0.3f;
	float3 ambient = float3(ambientLight, ambientLight, ambientLight);

	// diffuse light
//	float4 normal_vector = input.normal;

	float diffuseLight = max(dot(normal, lightDir), 0.0);

	float3 diffuse = diffuseLight * lightColor;

	int shininess = 128;
	float3 viewDir = normalize(viewPos - vec3(FragPos));
	float3 reflectDir = reflect(-lightDir, normalVector);
	float specularLight = max(dot(viewDir, reflectDir), 0.0);
	specularLight = pow(specularLight, shininess);
	vec3 specular = specularLight * lightColor;

	float4 result = float4((ambient + diffuse), 1.0f) * cColor;
	//vec4 result = vec4((ambient + diffuse + specular), 1.0f) * texture(texture1, TexCoord);
	//FragColor = result;
}



