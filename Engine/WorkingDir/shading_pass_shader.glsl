#ifdef SHADING_PASS_SHADER

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
	Light uLights[150];
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////


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

in vec2 vTexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gDepth;

layout(location = 0) out vec4 FragColor;

vec3 CalculateLighting(Light light, vec3 normal, vec3 view_dir, vec3 frag_pos, vec3 pixelColor);

void main()
{
	// retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, vTexCoord).rgb;
    vec3 Normal = texture(gNormal, vTexCoord).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, vTexCoord).rgb;
    float Specular = 0;

	vec3 viewDir  = normalize(uCameraPosition - FragPos);

    vec3 lighting  = vec3(0.0);
    for(int i = 0; i < uLightCount; ++i)
		lighting += CalculateLighting(uLights[i], Normal, viewDir, FragPos, Diffuse);

    FragColor = vec4(lighting, 1.0);
}

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec3 pixelColor)
{
	float intensity = float(light.intensity);
	intensity *= 0.01;

	// Diffuse 
	vec3 lightDirection = normalize(-light.direction);
	float diff = max(dot(lightDirection, normal), 0.0);
	vec3 diffuse = light.color * diff * pixelColor *  intensity;

	// Specular
	vec3 halfwayDir = normalize(lightDirection + view_dir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 128.0);
	vec3 specular = light.color * spec * intensity;

	vec3 result = (diffuse + specular);
	return result;
}

vec3 CalculatePointLight(Light light, vec3 normal, vec3 view_dir, vec3 frag_pos, vec3 pixelColor)
{
	float intensity = float(light.intensity);
	intensity *= 0.01;

	// Diffuse 
	vec3 lightDirection = normalize(light.position - frag_pos);
	float diff = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = light.color * diff * pixelColor * intensity;

	// Specular
    vec3 halfwayDir = normalize(lightDirection + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 128.0);
	vec3 specular = light.color * spec * intensity;

	// Attenuation
 	float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + 0.3 * distance + 0.5 * (distance * distance));    

	diffuse *= attenuation;
    specular *= attenuation;

	vec3 result = (diffuse + specular);
	return result;
}


vec3 CalculateLighting(Light light, vec3 normal, vec3 view_dir, vec3 frag_pos, vec3 pixelColor)
{
	vec3 result = vec3(0.0);

	switch(light.type)
	{
		case 0: 
			result = CalculatePointLight(light, normal, view_dir, frag_pos, pixelColor);
			break;
		case 1:
			result = CalculateDirectionalLight(light, normal, view_dir, pixelColor);
			break;

		default: break;
	}

	return result;
}

#endif
#endif