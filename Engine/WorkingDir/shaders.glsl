///////////////////////////////////////////////////////////////////////----------------------------------------------------------------------------------------------------

#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
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