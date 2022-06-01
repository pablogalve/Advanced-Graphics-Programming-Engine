#ifdef SKYBOX_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    //TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
    TexCoords = vec3(aPos.x, aPos.y, -aPos.z);
}  

#elif defined(FRAGMENT) ///////////////////////////////////////////////

#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}

#endif
#endif