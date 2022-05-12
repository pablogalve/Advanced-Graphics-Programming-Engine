//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>

#include "assimp_model_loading.h"
#include "buffer_management.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf_s(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    // Try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i) 
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    // Create a new vao for this submesh/program
    GLuint vaoHandle = 0;

    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    // We have to link all vertex inputs attributes to attributes in the vertex buffer
    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i) 
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }

        assert(attributeWasLinked); // The submesh should provide an attribute for each vertice inputs
    }

    glBindVertexArray(0);

    // Store it in the list of vaos for this submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    // Get OpenGL Information
    app->glInfo.version = (const char*)glGetString(GL_VERSION);
    app->glInfo.renderer = (const char*)glGetString(GL_RENDERER);
    app->glInfo.vendor = (const char*)glGetString(GL_VENDOR);
    app->glInfo.GLSLversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    // Camera
    {
        app->camera = Camera(
            glm::vec3(0.0f, 4.0f, 15.0f),          // Position
            glm::vec3(-90.0f, 0.0f, 0.0f)         // Rotation
        );

        app->camera.target = glm::vec3(glm::vec3(0.0f, 0.0f, 0.0f));
        app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
        app->camera.znear = 0.1f;
        app->camera.zfar = 1000.0f;
        app->camera.projection = glm::perspective(glm::radians(60.0f), app->camera.aspectRatio, app->camera.znear, app->camera.zfar);
        app->camera.viewMatrix = glm::lookAt(app->camera.position, app->camera.target, glm::vec3(0.0f, 1.0f, 0.0f));
    }    

    // Gameobjects - Entities
    {
        std::unique_ptr<Entity> plane = std::make_unique<Entity>(
            glm::vec3(10.0f, 1.5f, 0.0f), // Position
            glm::vec3(1.0f),             // Scale factor
            0,                            // Model index
            EntityType::PLANE             // Type
            );

        std::unique_ptr<Entity> patrick1 = std::make_unique<Entity>(
            glm::vec3(10.0f, 1.5f, 0.0f), // Position
            glm::vec3(1.0f),             // Scale factor
            0,                            // Model index
            EntityType::PATRICK           // Type
            );

        std::unique_ptr<Entity> patrick2 = std::make_unique<Entity>(
            glm::vec3(2.5f, 1.5f, 0.0f),  // Position
            glm::vec3(1.0f),             // Scale factor
            0,                            // Model index
            EntityType::PATRICK           // Type
            );

        std::unique_ptr<Entity> patrick3 = std::make_unique<Entity>(
            glm::vec3(0.0f, 1.5f, -2.0f), // Position
            glm::vec3(1.0f),             // Scale factor
            0,                            // Model index
            EntityType::PATRICK           // Type
            );

        app->entities.push_back(std::move(patrick1));
        app->entities.push_back(std::move(patrick2));
        app->entities.push_back(std::move(patrick3));
    }

    // Mode
    app->mode = Mode::Mode_TexturedMesh;

    switch (app->mode)
    {
        case Mode::Mode_TexturedQuad:
        {
            InitQuad(app);

            break;
        }
        case Mode::Mode_TexturedMesh:
        {
            InitPatrickModel(app);
            InitPlane(app);

            break;
        }
        case Mode::Mode_Count:
        {

            break;
        }
        default:
        {
            break;
        }
    }
}

void InitQuad(App* app)
{
    // Init verts and indices to draw quad
    VertexV3V2 vertices[] = {
        {glm::vec3( -0.5, -0.5,  0.0), glm::vec2( 0.0,  0.0) }, // bottom-left
        {glm::vec3(  0.5, -0.5,  0.0), glm::vec2( 1.0,  0.0) }, // bottom-right
        {glm::vec3(  0.5,  0.5,  0.0), glm::vec2( 1.0,  1.0) }, // top-right
        {glm::vec3( -0.5,  0.5,  0.0), glm::vec2( 0.0,  1.0) }, // top-left
    };
    
    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // Geometry
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Attribute state
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0, 3 }); // position
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2, 2 }); // texCoord

    app->programUniformTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");

    if (app->programUniformTexture == GL_INVALID_VALUE || app->programUniformTexture == GL_INVALID_OPERATION)
    {
        ILOG("ProgramUniformTexture loaded incorrectly");
    }

    // Texture initialization
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
}

void InitPatrickModel(App* app)
{
    app->patrickTexIdx = LoadModel(app, "Patrick/Patrick.obj");

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "GEOMETRY_PASS_SHADER");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];

    int attributeCount = 0;
    glGetProgramiv(texturedMeshProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    for (int i = 0; i < attributeCount; ++i)
    {
        GLchar attribName[100];
        int attribNameLength = 0, attribSize = 0;
        GLenum attribType;

        glGetActiveAttrib(texturedMeshProgram.handle, i, ARRAY_COUNT(attribName), &attribNameLength, &attribSize, &attribType, attribName);

        int attributeLocation = glGetAttribLocation(texturedMeshProgram.handle, attribName);

        texturedMeshProgram.vertexInputLayout.attributes.push_back({ (u8)attributeLocation, (u8)attribSize });
    }

    // Uniforms initialization
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    glGenBuffers(1, &app->uniformBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void InitPlane(App* app)
{
    std::vector<float> vertices = {
        -1.0, -1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, // bottom-left
         1.0, -1.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, // bottom-right
         1.0,  1.0, 0.0, 0.0, 0.0, -1.0, 1.0, 1.0, // top-right
        -1.0,  1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0  // top-left
    };

    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };


}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);

    // OpenGL information
    ImGui::Text("OpenGL version: %s", app->glInfo.version.c_str());
    ImGui::Text("OpenGL renderer: %s", app->glInfo.renderer.c_str());
    ImGui::Text("GPU vendor: %s", app->glInfo.vendor.c_str());
    ImGui::Text("GLSL version: %s", app->glInfo.GLSLversion.c_str());

    // Camera
    ImGui::Separator();
    ImGui::Text("Camera");

    ImGui::Text("Movement speed");
    ImGui::DragFloat("X", &app->camera.speed);

    ImGui::Text("Position");
    ImGui::PushItemWidth(100);
    ImGui::DragFloat("X", &app->camera.position.x);
    ImGui::SameLine();
    ImGui::DragFloat("Y", &app->camera.position.y);
    ImGui::SameLine();
    ImGui::DragFloat("Z", &app->camera.position.z);
    ImGui::PopItemWidth();

    ImGui::Text("Rotation");
    ImGui::PushItemWidth(100);
    ImGui::DragFloat("Yaw", &app->camera.yaw);
    ImGui::SameLine();
    ImGui::DragFloat("Pitch", &app->camera.pitch);
    ImGui::PopItemWidth();

    ImGui::End();      
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    app->camera.Update(app);

    // Global parameters
    MapBuffer(app->globalBuffer, GL_WRITE_ONLY);
    app->globalParamsOffset = app->globalBuffer.head;

    //PushVec3(app->globalBuffer, app->camera.position);

    app->globalParamsSize = app->globalBuffer.head - app->globalParamsOffset;

    UnmapBuffer(app->globalBuffer);

    // Update uniform
    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBuffer.handle);
    u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    u32 bufferHead = 0;



    // Entities
    for (int i = 0; i < app->entities.size(); ++i)
    {
        bufferHead = Align(bufferHead, app->uniformBlockAlignment);

        app->entities[i]->localParamsOffset = bufferHead;

        memcpy(bufferData + bufferHead, glm::value_ptr(app->entities[i]->worldMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        glm::mat4 worldViewProjectionMatrix = app->camera.projection * app->camera.viewMatrix * app->entities[i]->worldMatrix;
        memcpy(bufferData + bufferHead, glm::value_ptr(worldViewProjectionMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        app->entities[i]->localParamsSize = bufferHead - app->entities[i]->localParamsOffset;
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Render(App* app)
{
    // Clear the screen (also ImGui...)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);
    glEnable(GL_DEPTH_TEST);

    switch (app->mode)
    {
        case Mode::Mode_TexturedQuad:
        {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& programTexturedGeometry = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(programTexturedGeometry.handle);
            glBindVertexArray(app->vao);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
                
            break;
        }
        case Mode::Mode_TexturedMesh:
        {
            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            if (app->models.size() == 0) {
                throw std::invalid_argument("There are no models. Check if there are models in the directory and if LoadModel() is called.");
            }

            for (int i = 0; i < app->entities.size(); ++i)
            {
                Model& model = app->models[app->entities[i]->modelIndex];
                Mesh& mesh = app->meshes[model.meshIdx];
                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuffer.handle, app->entities[i]->localParamsOffset, app->entities[i]->localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);

                    u32 subMeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[subMeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform1i(app->texturedMeshProgram_uTexture, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                }
            }

            break;
        }
        case Mode::Mode_Count:
        {

            break;
        }
        default:
        {
            break;
        }            
    }
}

