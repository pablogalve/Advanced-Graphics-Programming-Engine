#include "framebuffer.h"
#include "engine.h"

FrameBuffer::FrameBuffer()
{
}

FrameBuffer::~FrameBuffer()
{
	FreeMemory();
}

void FrameBuffer::Initialize(float displayWidth, float displayHeight)
{
	for (u32 i = 0; i < RenderTargetType::MAX; ++i)
	{
		IDs[i] = 0u;
	}

	ReserveMemory(displayWidth, displayHeight);
	UpdateFBO(displayWidth, displayHeight);
}

void FrameBuffer::Bind(bool aClear)
{
	glBindFramebuffer(GL_FRAMEBUFFER, IDs[FBO]);
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
	return IDs[textureType];
}

void GBuffer::ReserveMemory(float displayWidth, float displayHeight)
{
	// Here we reserve all the memory, to be later on easy to delete 
	glGenTextures(1, &IDs[DEFAULT]);
	glGenTextures(1, &IDs[POSITION]);
	glGenTextures(1, &IDs[NORMALS]);
	glGenTextures(1, &IDs[ALBEDO]);
	glGenTextures(1, &IDs[DEPTH]);
	
	glGenFramebuffers(1, &IDs[FBO]);
	glGenRenderbuffers(1, &IDs[ZBO]);
}

void GBuffer::FreeMemory()
{
	glDeleteTextures(1, &IDs[DEFAULT]);
	glDeleteTextures(1, &IDs[POSITION]);
	glDeleteTextures(1, &IDs[NORMALS]);
	glDeleteTextures(1, &IDs[ALBEDO]);
	glDeleteTextures(1, &IDs[DEPTH]);
	
	glDeleteFramebuffers(1, &IDs[FBO]);
	glDeleteRenderbuffers(1, &IDs[ZBO]);
}

void GBuffer::UpdateFBO(float displayWidth, float displayHeight)
{
	glBindTexture(GL_TEXTURE_2D, IDs[DEFAULT]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, displayWidth, displayHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, IDs[DEPTH]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displayWidth, displayHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, IDs[POSITION]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displayWidth, displayHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);


	glBindTexture(GL_TEXTURE_2D, IDs[NORMALS]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displayWidth, displayHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, IDs[ALBEDO]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayWidth, displayHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, IDs[FBO]);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, IDs[DEFAULT], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, IDs[POSITION], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, IDs[NORMALS], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, IDs[ALBEDO], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, IDs[DEPTH], 0);

	GLuint drawBuffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, drawBuffers);

	glBindRenderbuffer(GL_RENDERBUFFER, IDs[ZBO]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, displayWidth, displayHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, IDs[ZBO]);
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
	glGenTextures(1, &IDs[DEFAULT]);
	glGenTextures(1, &IDs[DEPTH]);

	glGenFramebuffers(1, &IDs[FBO]);
}

void ShadingBuffer::FreeMemory()
{
	glDeleteTextures(1, &IDs[DEFAULT]);
	glDeleteTextures(1, &IDs[DEPTH]);

	glDeleteFramebuffers(1, &IDs[FBO]);
}

void ShadingBuffer::UpdateFBO(float displayWidth, float displayHeight)
{
	glBindTexture(GL_TEXTURE_2D, IDs[DEFAULT]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, displayWidth, displayHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, IDs[DEPTH]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displayWidth, displayHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, IDs[FBO]);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, IDs[DEFAULT], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, IDs[DEPTH], 0);

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
