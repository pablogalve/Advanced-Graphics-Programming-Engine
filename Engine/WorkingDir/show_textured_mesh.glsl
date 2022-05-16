#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

struct Light {
	unsigned int type; 
	vec3 position;
	vec3 color; 
	vec3 direction;
	unsigned int intensity;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount; 
	Light uLights[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 viewDir;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	viewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 viewDir; 

uniform sampler2D uTexture;

struct Light {
	unsigned int type; 
	vec3 position;
	vec3 color; 
	vec3 direction;
	unsigned int intensity;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount; 
	Light uLights[16];
};

layout(location = 0) out vec4 FragColor;

vec3 CalculateLighting(Light light, vec3 normal, vec3 viewDir, vec3 frag_pos);

void main()
{
	vec3 lightColorInfluence = vec3(0.0);
	
	for(int i = 0; i < uLightCount; ++i)
		lightColorInfluence += CalculateLighting(uLights[i], vNormal, viewDir, vPosition);

	FragColor = texture(uTexture, vTexCoord) * vec4(lightColorInfluence, 1.0);
}

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir)
{
	// Diffuse 
	vec3 lightDirection = normalize(-light.direction);
	float diff = max(dot(lightDirection, normal), 0.0);
	vec3 diffuse = light.color * diff;

	// Specular
	vec3 specular = vec3(0.0);

	vec3 result = (diffuse + specular);
	return result;
}

vec3 CalculatePointLight(Light light, vec3 normal, vec3 view_dir, vec3 frag_pos)
{
	// Diffuse 
	vec3 lightDirection = normalize(light.position - frag_pos);
	float diff = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = light.color * diff;

	// Specular
	vec3 specular = vec3(0.0);

	// Attenuation
 	float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + 0.5 * distance + 1.0 * (distance * distance));    

	vec3 result = (diffuse + specular);
	return result;
}


vec3 CalculateLighting(Light light, vec3 normal, vec3 view_dir, vec3 frag_pos)
{
	vec3 result = vec3(0.0);

	switch(light.type)
	{
		case 0: 
			result = CalculatePointLight(light, normal, view_dir, frag_pos);
			break;
		case 1:
			result = CalculateDirectionalLight(light, normal, view_dir);
			break;

		default: break;
	}

	return result;
}
#endif
#endif