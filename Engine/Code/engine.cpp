#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>

#include "assimp_model_loading.h"
#include "buffer_management.h"
#include <iostream>

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
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(vertexShaderDefine),
        (GLint)programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(fragmentShaderDefine),
        (GLint)programSource.len
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
    GLenum dataFormat = GL_RGB;
    GLenum dataType = GL_UNSIGNED_BYTE;

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

void SetAttributes(Program& program)
{
    int attributeCount;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    GLchar attributeName[64];
    GLsizei attributeNameLength;
    GLint attributeSize;
    GLenum attributeType;

    for (int i = 0; i < attributeCount; ++i)
    {
        glGetActiveAttrib(program.handle, i, 64, &attributeNameLength, &attributeSize, &attributeType, attributeName);

        GLint attributeLocation = glGetAttribLocation(program.handle, attributeName);
        program.vertexInputLayout.attributes.push_back({ (u8)attributeLocation,(u8)attributeSize });
    }
}

void InitEntitiesInBulk(App* app, std::vector<glm::vec3> positions, u32 modelId, float scaleFactor)
{
    int nr_entities = positions.size();
    for (int i = 0; i < nr_entities; ++i)
    {
        Entity entity = Entity(
            positions[i],           // Position
            glm::vec3(scaleFactor), // Scale factor
            modelId               // Model index
        );

        app->entities.push_back(entity);
    }
}

void Init(App* app)
{
    // Get OpenGL Information
    app->glInfo.version = (const char*)glGetString(GL_VERSION);
    app->glInfo.renderer = (const char*)glGetString(GL_RENDERER);
    app->glInfo.vendor = (const char*)glGetString(GL_VENDOR);
    app->glInfo.GLSLversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
       
    // Program 
    /*app->texturedGeometryProgramIdx = LoadProgram(app, "textured_geometry_shader.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    app->geometryPassShaderId = LoadProgram(app, "geometry_pass_shader.glsl", "GEOMETRY_PASS_SHADER");
    Program& geometryPassShader = app->programs[app->geometryPassShaderId];
    app->programGPassUniformTexture = glGetUniformLocation(geometryPassShader.handle, "uTexture");
    SetAttributes(geometryPassShader);

    // Program
    app->shadingPassShaderId = LoadProgram(app, "shading_pass_shader.glsl", "SHADING_PASS_SHADER");
    Program& shadingPassShader = app->programs[app->shadingPassShaderId];
    app->programShadingPassUniformTexturePosition = glGetUniformLocation(shadingPassShader.handle, "gPosition");
    app->programShadingPassUniformTextureNormals = glGetUniformLocation(shadingPassShader.handle, "gNormal");
    app->programShadingPassUniformTextureAlbedo = glGetUniformLocation(shadingPassShader.handle, "gAlbedoSpec");
    app->programShadingPassUniformTextureDepth = glGetUniformLocation(shadingPassShader.handle, "gDepth");
    SetAttributes(shadingPassShader);
    
    app->lightsShaderId = LoadProgram(app, "lights_shader.glsl", "LIGHTS_SHADER");
    Program& lightsShader = app->programs[app->lightsShaderId];
    app->programLightsUniformColor = glGetUniformLocation(lightsShader.handle, "lightColor");
    app->programLightsUniformWorldMatrix = glGetUniformLocation(lightsShader.handle, "uWorldViewProjectionMatrix");
    SetAttributes(lightsShader);

    app->texturedMeshProgramIdx = LoadProgram(app, "show_textured_mesh.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    SetAttributes(texturedMeshProgram);*/

    InitSkybox(app);
    InitWaterShader(app);
    

    // Camera    
    app->camera = Camera(
        glm::vec3(0.0f, 4.0f, 15.0f),          // Position
        glm::vec3(-90.0f, 0.0f, 0.0f)          // Rotation
    );

    app->camera.target = glm::vec3(glm::vec3(0.0f, 0.0f, 0.0f));
    app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    app->camera.znear = 0.1f;
    app->camera.zfar = 1000.0f;
    app->camera.projection = glm::perspective(glm::radians(60.0f), app->camera.aspectRatio, app->camera.znear, app->camera.zfar);
    app->camera.viewMatrix = glm::lookAt(app->camera.position, app->camera.target, glm::vec3(0.0f, 1.0f, 0.0f));
            
    // Init models
    /*u32 patrickTexIdx = LoadModel(app, "Models/Patrick/Patrick.obj");
    app->planeId = LoadModel(app, "Models/Wall/plane.obj");
    app->sphereId = LoadModel(app, "Models/Sphere/sphere.obj");
    u32 cyborgId = LoadModel(app, "Models/Cyborg/cyborg.obj");
    u32 planetMarsId = LoadModel(app, "Models/Planet/Mars/mars.obj");
    u32 woodenCartId = LoadModel(app, "Models/WoodenCart/cart_OBJ.obj");
        
    // Gameobjects - Entities and lights
    {
        std::vector<glm::vec3> groundPos = { glm::vec3(0.0f, -2.0f, 0.0f) };
        std::vector<glm::vec3> marsPos = { glm::vec3(0.0f, -127.5f, 0.0f)};
        std::vector<glm::vec3> woodenCartPos = { glm::vec3(0.0f, 0.0f, 0.0f)};

        // Add patricks
        std::vector<glm::vec3> patricksPos = {
            glm::vec3(2.0f, 1.5f, 15.0f),
            glm::vec3(7.0f, 1.5f, 22.0f),
            glm::vec3(3.0f, 1.5f, 30.0f),
            glm::vec3(-5.0f, 1.5f, 30.0f),
            glm::vec3(-16.0f, 1.5f, -15.0f),
        };

        // Add cyborgs
        std::vector<glm::vec3> cyborgsPos =
        {
            glm::vec3(1.0f, -2.0f, 3.0f),
            glm::vec3(15.0f, -2.0f, 3.0f),
            glm::vec3(10.0f, -2.0f, 3.0f),
            glm::vec3(1.0f, -2.0f, 30.0f),
            glm::vec3(6.0f, -2.0f, 15.0f),
        };

        //InitEntitiesInBulk(app, groundPos, app->planeId, 1.0f);
        InitEntitiesInBulk(app, patricksPos, patrickTexIdx, 1.0f);
        InitEntitiesInBulk(app, cyborgsPos, cyborgId, 2.0f);
        InitEntitiesInBulk(app, marsPos, planetMarsId, 37.5f);
        InitEntitiesInBulk(app, woodenCartPos, woodenCartId, 3.0f);
    }*/

    // Add lights
    /*const u32 nr_lights = 15;
    glm::vec3 pointLightsPos[nr_lights] =
    {
        glm::vec3(-100.0f, -50.0f, -10.0f),  // Around planet surfice
        glm::vec3(-75.0f, -25.0f, -10.0f),   // Around planet surfice
        glm::vec3(-50.0f, -10.0f, -10.0f),   // Around planet surfice
        glm::vec3(0.0f, 5.0f, 0.0f),         // Inside the the wooden cart
        glm::vec3(2.0f, 1.5f, 16.0f),        // Illuminating Patrick 
        glm::vec3(7.0f, 1.5f, 20.0f),        // Illuminating Patrick 
        glm::vec3(3.0f, 1.5f, 32.0f),        // Illuminating Patrick 
        glm::vec3(-5.0f, 1.5f, 28.0f),       // Illuminating Patrick 
        glm::vec3(-16.0f, 1.5f, -13.0f),     // Illuminating Patrick 
        glm::vec3(1.0f, 6.0f, 5.0f),         // Illuminating cyborg 
        glm::vec3(15.0f, 6.0f, 1.0f),        // Illuminating cyborg 
        glm::vec3(10.0f, 6.0f, 5.0f),        // Illuminating cyborg 
        glm::vec3(1.0f, 6.0f, 32.0f),        // Illuminating cyborg 
        glm::vec3(6.0f, 6.0f, 13.0f),        // Illuminating cyborg 
    };
    
    for (int i = 0; i < nr_lights; i++)
    {
        Light pointLight = Light(
            LightType::LightType_Point,
            glm::vec3(1.0f, 0.5f, 0.0f),   // Color
            glm::vec3(0.0f, -1.0, -1.0f),  // Direction
            pointLightsPos[i],             // Position
            200U                            // Intensity
        );

        app->lights.push_back(pointLight);
    }

    Light directionalLight1 = Light(
        LightType::LightType_Directional,
        glm::vec3(0.5f, 0.5f, 0.5f),    // Color
        glm::vec3(-0.5f, -0.5, 0.5f),   // Direction
        glm::vec3(0.1f, 20.0f, 0.1f),   // Position
        30U                             // Intensity
        );
    Light directionalLight2 = Light(
        LightType::LightType_Directional,
        glm::vec3(0.5f, 0.5f, 0.5f),    // Color
        glm::vec3(-1.0f, -0.5, 0.0f),   // Direction
        glm::vec3(0.1f, 20.0f, 0.1f) ,  // Position
        50U                             // Intensity
        );
    app->lights.push_back(directionalLight1);
    app->lights.push_back(directionalLight2);
    */
    // Local parameters
    /*glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBufferAlignment);
    app->uniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // Global parameters
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxGlobalParamsBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->globalParamsAlignment);
    app->globalBuffer = CreateConstantBuffer(app->maxGlobalParamsBufferSize);

    // FBO
    app->gFbo.Initialize(app->displaySize.x, app->displaySize.y);
    app->shadingFbo.Initialize(app->displaySize.x, app->displaySize.y);*/

app->renderWater = false;
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
    ImGui::DragFloat("Speed", &app->camera.speed, 0.01f, 0.1f, 2.0f);

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

    ImGui::Separator();
    ImGui::Text("Orbiting around a pivot point");
    ImGui::Checkbox("Orbital camera", &app->camera.orbiting);
    ImGui::DragFloat("Radius", &app->camera.radius);
    ImGui::DragFloat("Rotation speed", &app->camera.rotationSpeed, 0.01f, -0.5f, 0.5f);
    ImGui::Text("Camera Target");
    ImGui::PushItemWidth(100);
    ImGui::DragFloat("Xt", &app->camera.target.x);
    ImGui::SameLine();
    ImGui::DragFloat("Yt", &app->camera.target.y);
    ImGui::SameLine();
    ImGui::DragFloat("Zt", &app->camera.target.z);
    ImGui::PopItemWidth();

    // Render target
    ImGui::Separator();
    ImGui::Text("Render Targets - gBuffer");
    const char* items[] = { "Default", "Position", "Normals", "Albedo", "Depth" };
    static int item = 0;
    if (ImGui::Combo("Render Target", &item, items, IM_ARRAYSIZE(items)))
    {
        app->renderTarget = (RenderTargetType)item;
    }

    ImGui::Separator();
    ImGui::Checkbox("Enable debug groups", &app->enableDebugGroup);
    ImGui::Separator();
    ImGui::Checkbox("Enable Water Rendering", &app->renderWater);

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    app->camera.HandleInput(app);

    // Global parameters
    /*MapBuffer(app->globalBuffer, GL_WRITE_ONLY);
    app->globalParamsOffset = app->globalBuffer.head;

    PushVec3(app->globalBuffer, app->camera.position);
    PushUInt(app->globalBuffer, app->lights.size());

    // Lights
    for (int i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->globalBuffer, sizeof(glm::vec4));

        Light& light = app->lights[i];
        PushUInt(app->globalBuffer, static_cast<u32>(light.type));
        PushVec3(app->globalBuffer, light.position);
        PushVec3(app->globalBuffer, light.color);
        PushVec3(app->globalBuffer, light.direction);
        PushUInt(app->globalBuffer, light.intensity);
    }

    app->globalParamsSize = app->globalBuffer.head - app->globalParamsOffset;

    UnmapBuffer(app->globalBuffer);


    MapBuffer(app->uniformBuffer, GL_WRITE_ONLY);

    // Entities
    for (int i = 0; i < app->entities.size(); ++i)
    {
        AlignHead(app->uniformBuffer, app->uniformBufferAlignment);

        glm::mat4 worldMatrix = app->entities[i].worldMatrix;
        glm::mat4 worldViewProjectionMatrix = app->camera.projection * app->camera.viewMatrix * worldMatrix;

        app->entities[i].localParamsOffset = app->uniformBuffer.head;
        PushMat4(app->uniformBuffer, worldMatrix);
        PushMat4(app->uniformBuffer, worldViewProjectionMatrix);
        app->entities[i].localParamsSize = app->uniformBuffer.head - app->entities[i].localParamsOffset;
    }

    UnmapBuffer(app->uniformBuffer);*/
}

void Render(App* app)
{
    // Clear the screen (also ImGui...)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);
        glEnable(GL_DEPTH_TEST);
    }

    // Render skybox
    RenderSkybox(app);

    // Render object
    /* {
        app->gFbo.Bind();

        Program& texturedMeshProgram = app->programs[app->geometryPassShaderId];
        glUseProgram(texturedMeshProgram.handle);

        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

        if (app->models.size() == 0) {
            throw std::invalid_argument("There are no models. Check if there are models in the directory and if LoadModel() is called.");
        }

        for (int i = 0; i < app->entities.size(); ++i)
        {
            if (app->enableDebugGroup)
            {
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Entity");
            }

            Model& model = app->models[app->entities[i].modelIndex];
            Mesh& mesh = app->meshes[model.meshIdx];
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuffer.handle, app->entities[i].localParamsOffset, app->entities[i].localParamsSize);

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

            if (app->enableDebugGroup)
            {
                glPopDebugGroup();
            }
        }

        app->gFbo.Unbind();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);*/
    /*
    // Shading pass

    glDisable(GL_DEPTH_TEST);

    app->shadingFbo.Bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Program& shaderPassProgram = app->programs[app->shadingPassShaderId];
    glUseProgram(shaderPassProgram.handle);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->globalBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

    glUniform1i(app->programShadingPassUniformTexturePosition, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->gFbo.GetTexture(RenderTargetType::POSITION));

    glUniform1i(app->programShadingPassUniformTextureNormals, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, app->gFbo.GetTexture(RenderTargetType::NORMALS));

    glUniform1i(app->programShadingPassUniformTextureAlbedo, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, app->gFbo.GetTexture(RenderTargetType::ALBEDO));

    glUniform1i(app->programShadingPassUniformTextureDepth, 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, app->gFbo.GetTexture(RenderTargetType::DEPTH));
    
    RenderQuad(app);    

    app->shadingFbo.Unbind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);

    // Render lights on top of the scene
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, app->gFbo.GetTexture(RenderTargetType::FBO));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->shadingFbo.GetTexture(RenderTargetType::FBO));
    glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    app->shadingFbo.Bind(false);

    Program& lightsShader = app->programs[app->lightsShaderId];
    glUseProgram(lightsShader.handle);

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        if (app->enableDebugGroup)
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Light");
        }

        // Render directional lights as planes and point lights as spheres
        u32 modelIndex = 0U;
        glm::mat4 worldMatrix;
        switch (app->lights[i].type)
        {
            case LightType::LightType_Directional:
                modelIndex = app->planeId;
                worldMatrix = TransformPositionScale(app->lights[i].position, glm::vec3(3.0f, 3.0f, 3.0f));
                worldMatrix = TransformRotation(worldMatrix, 90.0, glm::vec3(1.f, 0.f, 0.f));
                break;
            case LightType::LightType_Point:
                modelIndex = app->sphereId;
                worldMatrix = TransformPositionScale(app->lights[i].position, glm::vec3(0.3f, 0.3f, 0.3f));
                break;
        }

        glm::mat4 worldViewProjectionMatrix = app->camera.projection * app->camera.viewMatrix * worldMatrix;
        glUniformMatrix4fv(app->programLightsUniformWorldMatrix, 1, GL_FALSE, (GLfloat*)&worldViewProjectionMatrix);
        glUniform3f(app->programLightsUniformColor, app->lights[i].color.x, app->lights[i].color.y, app->lights[i].color.z);

        Mesh& mesh = app->meshes[app->models[modelIndex].meshIdx];
        GLuint vao = FindVAO(mesh, 0, lightsShader);
        glBindVertexArray(vao);

        glDrawElements(GL_TRIANGLES, mesh.submeshes[0].indices.size(), GL_UNSIGNED_INT, (void*)(u64)mesh.submeshes[0].indexOffset);

        if (app->enableDebugGroup)
        {
            glPopDebugGroup();
        }
    }

    app->shadingFbo.Unbind();

    Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
    glUseProgram(programTexturedGeometry.handle);

    glUniform1i(app->programUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);
    if (app->renderTarget == RenderTargetType::DEFAULT)
    {
        glBindTexture(GL_TEXTURE_2D, app->shadingFbo.GetTexture(RenderTargetType::DEFAULT));
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, app->gFbo.GetTexture(app->renderTarget));
    }
    
    RenderQuad(app);    

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);*/

    //RenderSkybox(app);    
    
    //Water rendering if enabled
    if (app->renderWater) {

        #pragma region REFLECTION_PASS

        
        //app->fboReflection->Bind(); BIND IN INIT

        Camera reflectionCamera = app->camera;

        reflectionCamera.position.y = -reflectionCamera.position.y;
        reflectionCamera.pitch = -reflectionCamera.pitch;
        reflectionCamera.viewportWidth = app->displaySize.x;
        reflectionCamera.viewportHeight = app->displaySize.y;
        //matrix

        passWaterScene(&reflectionCamera, GL_COLOR_ATTACHMENT0, WaterScenePart::Reflection);
        //passBackground(&reflectionCamera, GL_COLOR_ATTACHMENT0);----> means rendering everything else? no function in ppt;

        //app->fboReflection->FreeMemory(); UNBIND IN INIT
        #pragma endregion REFLECTION_PASS

        #pragma region REFRACTION_PASS
        //app->fboRefraction->Bind();

        Camera refractionCamera = app->camera;
        refractionCamera.viewportWidth = app->displaySize.x;
        refractionCamera.viewportWidth = app->displaySize.y;
        //matrix

        passWaterScene(&refractionCamera, GL_COLOR_ATTACHMENT0, WaterScenePart::Refraction);

        //app->fboRefraction->FreeMemory();
        #pragma endregion REFRACTION_PASS
     
    }
}

void RenderQuad(App* app)
{
    if (app->enableDebugGroup)
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "RenderQuad");
    }

    unsigned int quadVAO = 0;
    unsigned int quadVBO;

    if (quadVAO == 0)
    {
        float quadVertices[] =
        {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    if (app->enableDebugGroup)
    {
        glPopDebugGroup();
    }
}

void InitSkybox(App* app)
{
    app->skybox.shader = Shader("skybox.vert", "skybox.frag");
    app->skybox.shader.Activate();
    glUniform1i(glGetUniformLocation(app->skybox.shader.ID, "skybox"), 0);
    
    glEnable(GL_DEPTH_TEST); // Enables the Depth Buffer    
    glEnable(GL_CULL_FACE); // Enables Cull Facing    
    glCullFace(GL_FRONT); // Keeps front faces    
    glFrontFace(GL_CCW); // Uses counter clock-wise standard

    // Skybox VAO, VBO and EBO
    glGenVertexArrays(1, &app->skybox.VAO);
    glGenBuffers(1, &app->skybox.VBO);
    glGenBuffers(1, &app->skybox.EBO);
    glBindVertexArray(app->skybox.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, app->skybox.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(app->skybox.vertices), &app->skybox.vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->skybox.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(app->skybox.indices), &app->skybox.indices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
    app->skybox.cubemapTextureId = loadCubemap(app->skybox.faces);
}

void RenderSkybox(App* app)
{
    if (app->enableDebugGroup)
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Skybox");
    }

    // Since the cubemap will always have a depth of 1.0, we need that equal sign so it doesn't get discarded
    glDepthFunc(GL_LEQUAL);

    app->skybox.shader.Activate();
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    // We make the mat4 into a mat3 and then a mat4 again in order to get rid of the last row and column
    // The last row and column affect the translation of the skybox (which we don't want to affect)
    view = glm::mat4(glm::mat3(glm::lookAt(app->camera.position, app->camera.position + app->camera.direction, app->camera.up)));
    projection = glm::perspective(glm::radians(45.0f), (float)app->displaySize.x / app->displaySize.y, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(app->skybox.shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(app->skybox.shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the cubemap as the last object so we can save a bit of performance by discarding all fragments
    // where an object is present (a depth of 1.0f will always fail against any object's depth value)
    glBindVertexArray(app->skybox.VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->skybox.cubemapTextureId);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Switch back to the normal depth function
    glDepthFunc(GL_LESS);

    if (app->enableDebugGroup)
    {
        glPopDebugGroup();
    }
}

void InitWaterShader(App* app) {

    app->waterPassShaderID = LoadProgram(app, "water_shader.glsl", "WATER_PASS_SHADER");

    app->fboReflection = 0;
    app->fboRefraction = 0;

    GLuint rtReflection = 0;
    glGenTextures(1, &rtReflection);
    glBindTexture(GL_TEXTURE_2D, rtReflection);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLuint rtRefraction = 0;
    glGenTextures(1, &rtRefraction);
    glBindTexture(GL_TEXTURE_2D, rtRefraction);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLuint rtReflectionDepth = 0;
    glGenTextures(1, &rtReflectionDepth);
    glBindTexture(GL_TEXTURE_2D, rtReflectionDepth);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    GLuint rtRefractionDepth = 0;
    glGenTextures(1, &rtRefractionDepth);
    glBindTexture(GL_TEXTURE_2D, rtRefractionDepth);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    //FBO REFLECTION BIND->COLOR->DEPTH->RELEASE
    glGenFramebuffers(1, &app->fboReflection);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboReflection);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rtReflection, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rtReflectionDepth, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &app->fboReflection);

    //FBO REFRACTION BIND->COLOR->DEPTH->RELEASE
    glGenFramebuffers(1, &app->fboRefraction);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboRefraction);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rtRefraction, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rtRefractionDepth, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &app->fboRefraction);
}

void passWaterScene(App* app,Camera* camera, GLenum colorAttachment, WaterScenePart part) {

    glDrawBuffer(colorAttachment);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CLIP_DISTANCE0);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //shaderprogram
    Program& waterShaderProgram = app->programs[app->waterPassShaderID];
    glUseProgram(waterShaderProgram.handle);

    //Pass uniformvalues to the shader viewmatrix, projection, eyeworldspace...
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // These are very important to prevent seams
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, 
                GL_RGB, 
                width, 
                height, 
                0, 
                GL_RGB, 
                GL_UNSIGNED_BYTE, 
                data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }    

    return textureID;
}

u32 loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
