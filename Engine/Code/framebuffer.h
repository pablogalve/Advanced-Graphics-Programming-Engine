#pragma once

#include "platform.h"

enum RenderTargetType
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

	u32 IDs[RenderTargetType::MAX];
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