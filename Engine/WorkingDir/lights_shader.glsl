#ifdef LIGHTS_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uWorldViewProjectionMatrix;

void main()
{
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec3 lightColor; 

layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = vec4(lightColor, 1.0);
}

#endif
#endif