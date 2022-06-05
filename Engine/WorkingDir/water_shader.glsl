#ifdef WATER_PASS_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;

uniform mat4 projectionMatrix;
uniform mat4 worldViewMatrix;

out Data{
	vec3 positionViewspace;
	vec3 normalViewspace;
} VSOut;

void main(void)
{
	VSOut.positionViewspace = vec3(worldViewMatrix * vec4(position,1));
	VSOut.normalViewspace = vec3(worldViewMatrix * vec4(normal,0));
	gl_Position = projectionMatrix * vec4(VSOut.positionViewspace, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec2 viewportSize;
uniform mat4 modelViewMatrix;
uniform mat4 viewMatrixInv;
uniform mat4 projectionMatrixInv;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform sampler2D reflectionDepth;
uniform sampler2D refractionDepth;
uniform sampler2D normalMap;
uniform sampler2D dudvMap;

in Data{
	vec3 positionViewspace;
	vec3 normalViewspace;
} FSIn;

out vec4 outColor;

vec3 fresnelSchlick(float cosTheta, vec3 F0){
	return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 reconstructPixelPosition(float depth){
	vec2 texCoords = gl_FragCoord.xy / viewportSize;
	vec3 positionNDC = vec3(texCoords * 2.0 - vec2(1.0), depth * 2.0f - 1.0f);
	vec4 positionEyespace = projectionMatrixInv * vec4(positionNDC, 1.0f);
	positionEyespace.xyz /= positionEyespace.w;
	return positionEyespace.xyz;
}

void main()
{
	vec3 N = normalize(FSIn.normalViewspace);
	vec3 V = normalize(-FSIn.positionViewspace);
	vec3 Pw = vec3(viewMatrixInv * vec4(FSIn.positionViewspace, 1.0));
	vec2 texCoord = gl_FragCoord.xy / viewportSize;

	const vec2 waveLength = vec2(2.0f);
	const vec2 waveStrength = vec2(0.05f);
	const float turbidityDistance = 10.0f;

	vec2 distortion = (2.0 * texture(dudvMap, Pw.xz / waveLength).rg - vec2(1.0)) * waveStrength + waveStrength/7;

	vec2 reflectionTexCoord = vec2(texCoord.s, 1.0f - texCoord.t) + distortion;
	vec2 refractionTexCoord = texCoord + distortion;
	vec3 reflectionColor = texture(reflectionMap, reflectionTexCoord).rgb;
	vec3 refractionColor = texture(refractionMap, refractionTexCoord).rgb;

	float distortedGroundDepth = texture(refractionDepth, refractionTexCoord).x;
	vec3 distortedGroundPosViewspace = reconstructPixelPosition(distortedGroundDepth);
	float distortedWaterDepth = FSIn.positionViewspace.z - distortedGroundPosViewspace.z;
	float tintFactor = clamp(distotedWaterDepth / turbidityDistance, 0.0f, 1.0f);
	vec3 waterColor = vec3(0.25f, 0.4f, 0.6f);
	refractionColor = mix(refractionColor, waterColor, tintFactor);

	//Fresnel
	vec3 F0 = vec3(0.1);
	vec3 F = fresnelSchlick(max(0.0, dot(V,N))m F0);
	outColor.rgb = mix(fractionColor, reflectionColor, F);
	outColor.a = 1.0;
}
#endif
#endif