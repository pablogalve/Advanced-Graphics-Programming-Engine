#pragma once

#include "platform.h"

enum class RenderTargetType
{
	DEFAULT = 0,
	POSITION,
	NORMALS,
	ALBEDO,
	DEPTH,
	FBO,
	ZBO,
	MAX
};

class FrameBuffer
{
public:

	FrameBuffer();

	~FrameBuffer();

	void Initialize(float displayWidth, float displayHeight);

	virtual void ReserveMemory(float displayWidth, float displayHeight) = 0;
	virtual void FreeMemory() {}

	virtual void UpdateFBO(float displayWidth, float displayHeight) = 0;

	void Bind(bool aClear = true);
	void Unbind();

	u32 GetTexture(RenderTargetType textureType);

protected:

	u32 defaultTexture;
	u32 gBuffer, gPosition, gNormal, gAlbedoSpec, rboDepth;
	u32 zbo;
};

class GBuffer : public FrameBuffer
{
public:

	void ReserveMemory(float displayWidth, float displayHeight) override;
	void FreeMemory() override;

	void UpdateFBO(float displayWidth, float displayHeight) override;
};

class ShadingBuffer : public FrameBuffer
{
public:
	void ReserveMemory(float displayWidth, float displayHeight) override;
	void FreeMemory() override;

	void UpdateFBO(float displayWidth, float displayHeight) override;
};