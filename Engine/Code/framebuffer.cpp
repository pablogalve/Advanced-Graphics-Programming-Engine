#include "framebuffer.h"
#include "engine.h"
#include <stdexcept>

FrameBuffer::FrameBuffer()
{
}

FrameBuffer::~FrameBuffer()
{
	FreeMemory();
}

void FrameBuffer::Initialize(float displayWidth, float displayHeight)
{
	defaultTexture = 0u;
	gBuffer = 0u;
	gPosition = 0u;
	gNormal = 0u;
	gAlbedoSpec = 0u;
	rboDepth = 0u;
	zbo = 0u;

	ReserveMemory(displayWidth, displayHeight);
	UpdateFBO(displayWidth, displayHeight);
}

void FrameBuffer::Bind(bool aClear)
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	if (aClear)
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
}

void FrameBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

u32 FrameBuffer::GetTexture(RenderTargetType textureType)
{
	switch (textureType)
	{
	case RenderTargetType::DEFAULT:
		return defaultTexture;
	case RenderTargetType::POSITION:
		return gPosition;
	case RenderTargetType::NORMALS:
		return gNormal;
	case RenderTargetType::ALBEDO:
		return gAlbedoSpec;
	case RenderTargetType::DEPTH:
		return rboDepth;
	case RenderTargetType::FBO:
		return gBuffer;
	case RenderTargetType::ZBO:
		return zbo;
	default:
		throw std::invalid_argument("Called a texture that is not being handled in GetTexture()");
	}	
}

void GBuffer::ReserveMemory(float displayWidth, float displayHeight)
{
	glGenTextures(1, &defaultTexture);
	glGenTextures(1, &gPosition);
	glGenTextures(1, &gNormal);
	glGenTextures(1, &gAlbedoSpec);
	glGenTextures(1, &rboDepth);
	
	glGenFramebuffers(1, &gBuffer);
	glGenRenderbuffers(1, &zbo);
}

void GBuffer::FreeMemory()
{
	glDeleteTextures(1, &defaultTexture);
	glDeleteTextures(1, &gPosition);
	glDeleteTextures(1, &gNormal);
	glDeleteTextures(1, &gAlbedoSpec);
	glDeleteTextures(1, &rboDepth);
	
	glDeleteFramebuffers(1, &gBuffer);
	glDeleteRenderbuffers(1, &zbo);
}

void GBuffer::UpdateFBO(float displayWidth, float displayHeight)
{
	glBindTexture(GL_TEXTURE_2D, defaultTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, displayWidth, displayHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displayWidth, displayHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);


	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displayWidth, displayHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayWidth, displayHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, rboDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displayWidth, displayHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, defaultTexture, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gPosition, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gNormal, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gAlbedoSpec, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, rboDepth, 0);

	GLuint drawBuffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, drawBuffers);

	glBindRenderbuffer(GL_RENDERBUFFER, zbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, displayWidth, displayHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, zbo);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (frameBufferStatus)
		{
		case GL_FRAMEBUFFER_UNDEFINED: ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
		case GL_FRAMEBUFFER_UNSUPPORTED: ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
		default: ELOG("Unkown Framebuffer Status Error :)");
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void ShadingBuffer::ReserveMemory(float displayWidth, float displayHeight)
{
	glGenTextures(1, &defaultTexture);
	glGenTextures(1, &rboDepth);

	glGenFramebuffers(1, &gBuffer);
}

void ShadingBuffer::FreeMemory()
{
	glDeleteTextures(1, &defaultTexture);
	glDeleteTextures(1, &rboDepth);

	glDeleteFramebuffers(1, &gBuffer);
}

void ShadingBuffer::UpdateFBO(float displayWidth, float displayHeight)
{
	glBindTexture(GL_TEXTURE_2D, defaultTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, displayWidth, displayHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, rboDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displayWidth, displayHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, defaultTexture, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rboDepth, 0);

	GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (frameBufferStatus)
		{
		case GL_FRAMEBUFFER_UNDEFINED: ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
		case GL_FRAMEBUFFER_UNSUPPORTED: ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
		default: ELOG("Unkown Framebuffer Status Error :)");
		}
	}

	GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
