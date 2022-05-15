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

    // Gameobjects - Entities and lights
    {
        std::unique_ptr<Entity> plane = std::make_unique<Entity>(
            glm::vec3(10.0f, 1.5f, 0.0f), // Position
            glm::vec3(1.0f),              // Scale factor
            0,                            // Model index
            EntityType::PLANE             // Type
            );

        std::unique_ptr<Entity> patrick1 = std::make_unique<Entity>(
            glm::vec3(10.0f, 1.5f, 0.0f), // Position
            glm::vec3(1.0f),              // Scale factor
            0,                            // Model index
            EntityType::PATRICK           // Type
            );

        std::unique_ptr<Entity> patrick2 = std::make_unique<Entity>(
            glm::vec3(2.5f, 1.5f, 0.0f),  // Position
            glm::vec3(1.0f),              // Scale factor
            0,                            // Model index
            EntityType::PATRICK           // Type
            );

        std::unique_ptr<Entity> patrick3 = std::make_unique<Entity>(
            glm::vec3(0.0f, 1.5f, -2.0f), // Position
            glm::vec3(1.0f),              // Scale factor
            0,                            // Model index
            EntityType::PATRICK           // Type
            );

        app->entities.push_back(std::move(plane));
        app->entities.push_back(std::move(patrick1));
        app->entities.push_back(std::move(patrick2));
        app->entities.push_back(std::move(patrick3));

        std::shared_ptr<Light> light1 = std::make_unique<Light>(
            LightType::LightType_Directional,
            glm::vec3(0.5f, 0.5f, 0.5f), // Color
            glm::vec3(0.0f, -0.5, 0.5f), // Direction
            glm::vec3(0.1f, 20.0f, 0.1f)  // Position
            );

        app->lights.push_back(light1);
    }

    // Local parameters
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBufferAlignment);
    app->uniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // Global parameters
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxGlobalParamsBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->globalParamsAlignment);
    app->globalBuffer = CreateConstantBuffer(app->maxGlobalParamsBufferSize);

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
        case Mode::Mode_Plane:
        {
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
    app->patrickTexIdx = LoadModel(app, "Models/Patrick/Patrick.obj");

    app->texturedMeshProgramIdx = LoadProgram(app, "geometry_pass_shader.glsl", "GEOMETRY_PASS_SHADER");
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
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBufferAlignment);

    glGenBuffers(1, &app->uniformBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void InitPlane(App* app)
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\0";

    float vertices[] = {
        0.5f,  0.5f, 0.0f,  // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };   

    unsigned int indices[] = {
        0, 1, 3,    // first triangle
        1, 2, 3     // second triangle
    };
    
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------


    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // Save values in plane struct
    app->plane.VAO = VAO;    
    app->plane.shaderProgram = shaderProgram;
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

    // Render target
    ImGui::Separator();
    ImGui::Text("Render Targets");
    const char* items[] = { "Albedo", "Normals", "Position", "Depth"};
    static int item = 0;
    if (ImGui::Combo("Render Target", &item, items, IM_ARRAYSIZE(items)))
    {
        app->renderTarget = (RenderTargetType)item;
    }

    ImGui::Separator();
    ImGui::Checkbox("Enable debug groups", &app->enableDebugGroup);

    ImGui::End();      
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    app->camera.Update(app);

    // Global parameters
    MapBuffer(app->globalBuffer, GL_WRITE_ONLY);
    app->globalParamsOffset = app->globalBuffer.head;

    PushVec3(app->globalBuffer, app->camera.position);
    PushUInt(app->globalBuffer, app->lights.size());

    // Lights
    for (int i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->globalBuffer, sizeof(glm::vec4));

        Light& light = *app->lights[i];
        PushUInt(app->globalBuffer, light.type);
        PushVec3(app->globalBuffer, light.color);
        PushVec3(app->globalBuffer, light.direction);
        PushVec3(app->globalBuffer, light.position);
    }

    app->globalParamsSize = app->globalBuffer.head - app->globalParamsOffset;

    UnmapBuffer(app->globalBuffer);


    MapBuffer(app->uniformBuffer, GL_WRITE_ONLY);

    // Entities
    for (int i = 0; i < app->entities.size(); ++i)
    {
        //app->uniformBuffer.head = Align(app->uniformBuffer.head, app->uniformBlockAlignment);
        AlignHead(app->uniformBuffer, app->uniformBufferAlignment);

        glm::mat4 world = app->entities[i]->worldMatrix;
        glm::mat4 worldViewProjection = app->camera.projection * app->camera.viewMatrix * world;

        app->entities[i]->localParamsOffset = app->uniformBuffer.head;
        PushMat4(app->uniformBuffer, world);
        PushMat4(app->uniformBuffer, worldViewProjection);
        app->entities[i]->localParamsSize = app->uniformBuffer.head - app->entities[i]->localParamsOffset;
    }

    UnmapBuffer(app->uniformBuffer);
}

void Render(App* app)
{
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
            // Clear the screen (also ImGui...)
            {
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glViewport(0, 0, app->displaySize.x, app->displaySize.y);
                glEnable(GL_DEPTH_TEST);
            }            

            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            if (app->models.size() == 0) {
                throw std::invalid_argument("There are no models. Check if there are models in the directory and if LoadModel() is called.");
            }

            for (int i = 0; i < app->entities.size(); ++i)
            {
                if (app->enableDebugGroup)
                {
                    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Shaded model");
                }

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

                if (app->enableDebugGroup)
                {
                    glPopDebugGroup();
                }
            }

            break;
        }
        case Mode::Mode_Plane:
        {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(app->plane.shaderProgram);
            glBindVertexArray(app->plane.VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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