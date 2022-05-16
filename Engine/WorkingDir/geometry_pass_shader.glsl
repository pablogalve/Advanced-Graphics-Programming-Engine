#ifdef GEOMETRY_PASS_SHADER

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

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec3 vPosition; // in worldspace
out vec3 vNormal; // in worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition; // in worldspace
in vec3 vNormal; // in worldspace

uniform sampler2D uTexture;

layout(location = 0) out vec4 FragColor;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;
layout (location = 3) out vec3 gAlbedoSpec;
layout(location = 4) out vec4 gDepth;

float near = 0.1;
float far = 100.0;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
	gPosition = vPosition; 
	gNormal = normalize(vNormal);
	gAlbedoSpec.rgb = texture(uTexture, vTexCoord).rgb;
	gDepth = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
	FragColor = texture(uTexture, vTexCoord);
}

#endif
#endif