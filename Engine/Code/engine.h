//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#define BINDING(b) b

#include <glad/glad.h>
#include <memory>

#include "platform.h"
#include "camera.h"
#include "buffer_management.h"
#include "entity.h"
#include "framebuffer.h"
#include "Shader.h"
#include "Model.h"

struct Buffer
{
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void*   data;
};

struct Image
{
    void*      pixels;
    glm::ivec2 size;
    i32        nchannels;
    i32        stride;
};

struct TextureStruct
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

struct MeshStruct
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct ModelStruct
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

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    Light() {};
    Light(LightType type, glm::vec3 color, glm::vec3 direction, glm::vec3 position, unsigned int intensity) : 
        type(type), color(color), direction(direction), position(position), intensity(intensity) {};

    LightType type;
    glm::vec3 color;
    glm::vec3 direction;
    glm::vec3 position;
    unsigned int intensity; // From 0 to 100
};

struct Skybox
{
    Shader shader;
    u32 cubemapTextureId;
    float vertices[24] = 
    {        
        // Coordinates    
        -1.0f, -1.0f,  1.0f,//       7--------6
         1.0f, -1.0f,  1.0f,//      /|       /|
         1.0f, -1.0f, -1.0f,//     4--------5 |
        -1.0f, -1.0f, -1.0f,//     | |      | |
        -1.0f,  1.0f,  1.0f,//     | 3------|-2
         1.0f,  1.0f,  1.0f,//     |/       |/
         1.0f,  1.0f, -1.0f,//     0--------1
        -1.0f,  1.0f, -1.0f
        
    };
    unsigned int indices[36] = 
    {
        // Right
        1, 2, 6,
        6, 5, 1,
        // Left
        0, 4, 7,
        7, 3, 0,
        // Top
        4, 5, 6,
        6, 7, 4,
        // Bottom
        0, 3, 2,
        2, 1, 0,
        // Back
        0, 1, 5,
        5, 4, 0,
        // Front
        3, 7, 6,
        6, 2, 3
    };
    unsigned int VAO, VBO, EBO;
    std::vector<std::string> faces =
    {
        "Textures/skybox/right.jpg",
        "Textures/skybox/left.jpg",
        "Textures/skybox/top.jpg",
        "Textures/skybox/bottom.jpg",
        "Textures/skybox/front.jpg",
        "Textures/skybox/back.jpg",
    };
};

enum WaterScenePart {
    Reflection,
    Refraction
};

struct Object
{
    Model model;
    Shader shader;
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
    std::vector<Entity> entities;
    std::vector<Light> lights;

    std::vector<TextureStruct>  textures;
    std::vector<MeshStruct>     meshes;
    std::vector<ModelStruct>    models;
    std::vector<Material> materials;
    std::vector<Program>  programs;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    u32 geometryPassShaderId;
    u32 shadingPassShaderId;
    u32 lightsShaderId;
    
    // model id
    u32 planeId;
    u32 sphereId;

    // OpenGL info
    GLInfo glInfo;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
    GLuint programGPassUniformTexture;
    GLuint programShadingPassUniformTexturePosition;
    GLuint programShadingPassUniformTextureNormals;
    GLuint programShadingPassUniformTextureAlbedo;
    GLuint programShadingPassUniformTextureDepth;
    GLuint programLightsUniformColor;
    GLuint programLightsUniformWorldMatrix;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Local params
    Buffer uniformBuffer;    
    GLint maxUniformBufferSize;
    GLint uniformBufferAlignment;
    GLint texturedMeshProgram_uTexture;

    // Global params
    Buffer  globalBuffer;
    GLint   maxGlobalParamsBufferSize;
    GLint   globalParamsAlignment;
    u32     globalParamsOffset;
    u32     globalParamsSize;

    bool enableDebugGroup = true;
        
    // FBO - Deferred Rendering
    bool enableDeferredShading;
    GBuffer gFbo;
    ShadingBuffer shadingFbo;
    RenderTargetType renderTarget = RenderTargetType::DEFAULT;

    //Skybox Shader
    Skybox skybox;

    //Water Shader
    GLuint fboReflection;
    GLuint fboRefraction;

    GLuint waterPassShaderID;

    bool renderWater;

    Object backpack;
    Object water;
};

void Init(App* app);
void InitModelsAndLights(App* app);
void InitBackPack(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void RenderQuad(App* app);

//Skybox shader
void InitSkybox(App* app);
void RenderSkybox(App* app);
unsigned int loadCubemap(std::vector<std::string> faces);

void RenderDeferredRenderingScene(App* app);
void RenderForwardRenderingScene(App* app);

//Water Shader
void InitWaterShader(App* app);
void RenderWater(App* app);
void PassWaterScene(App* app, Camera* camera, GLenum colorAttachment, WaterScenePart part);

//Engine stuff
u32 loadTexture(char const* path);
u32 LoadTexture2D(App* app, const char* filepath);
GLuint FindVAO(MeshStruct& mesh, u32 submeshIndex, const Program& program);
void SetAttributes(Program& program);
void InitEntitiesInBulk(App* app, std::vector<glm::vec3> positions, u32 modelId, float scaleFactor = 1.0f);