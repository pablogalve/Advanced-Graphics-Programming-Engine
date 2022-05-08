///////////////////////////////////////////////////////////////////////----------------------------------------------------------------------------------------------------

#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;

	// We will usually not define the clipping scale manually
	// it is usually computed by the projection matrix. Because
	// we are not passing uniform transforms yet, we inscrease 
	// the clipping scale so that Patrick fits the screen.
	float clippingScale = 5.0;

	gl_Position = vec4(aPosition, clippingScale);

	// Patrick looks away from the camera by default, so I flip it here
	gl_Position.z = -gl_Position.z;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////----------------------------------------------------------------------------------------------------

#ifdef GEOMETRY_PASS_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

struct Light {
	unsigned int type; 
	vec3 position;
	vec3 color; 
	vec3 direction;
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

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

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
	float depth = LinearizeDepth(gl_FragCoord.z) / far;
	gDepth = vec4(vec3(depth), 1.0);
	FragColor = texture(uTexture, vTexCoord);
}

#endif
#endif