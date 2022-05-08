//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#define BINDING(b) b

#include <glad/glad.h>
#include <memory>

#include "platform.h"
#include "Camera.h"
#include "buffer_management.h"

struct Buffer
{
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void* data;
};

struct Image
{
    void*      pixels;
    glm::ivec2 size;
    i32        nchannels;
    i32        stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Material
{
    std::string name;
    glm::vec3   albedo;
    glm::vec3   emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32>   indices;
    u32                vertexOffset;
    u32                indexOffset;

    std::vector<Vao>   vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute>  attributes;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

struct GLInfo
{
    std::string version;
    std::string renderer;
    std::string vendor;
    std::string GLSLversion;
};

enum class Mode
{
    Mode_TexturedQuad,
    Mode_TexturedMesh,
    Mode_Count
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Entity
{
    glm::mat4 worldMatrix;  // Coordinates of an object with respect to the world space
    u32       modelIndex;
    u32       localParamsOffset;
    u32       localParamsSize;
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    glm::ivec2 displaySize;

    Camera camera;    
    std::vector <std::unique_ptr<Entity>> entities;

    std::vector<Texture>  textures;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;
    std::vector<Material> materials;
    std::vector<Program>  programs;

    // program indices
    u32 texturedMeshProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    u32 patrickTexIdx;

    // Mode
    Mode mode;

    // OpenGL info
    GLInfo glInfo;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;
    GLint texturedMeshProgram_uTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Local params
    Buffer uniformBuffer;    

    // Global params
    Buffer  globalBuffer;
    u32     globalParamsOffset;
    u32     globalParamsSize;
};

void Init(App* app);

void InitQuad(App* app);

void InitMesh(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

glm::mat4 TransformScale(const glm::vec3& scaleFactors);

glm::mat4 TransformPositionScale(const glm::vec3& pos, const glm::vec3& scaleFactors);