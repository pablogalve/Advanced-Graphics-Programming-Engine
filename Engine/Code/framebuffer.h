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

	void Initialize(float _width, float _height);

	virtual void ReserveMemory() = 0;
	virtual void FreeMemory() {}

	virtual void UpdateFBO() = 0;

	void Bind(bool aClear = true);
	void Unbind();

	u32 GetTexture(RenderTargetType textureType);

protected:

	float width = 0.f;
	float height = 0.f;

	u32 IDs[RenderTargetType::MAX];
};

class GBuffer : public FrameBuffer
{
public:

	void ReserveMemory() override;
	void FreeMemory() override;

	void UpdateFBO() override;
};

class ShadingBuffer : public FrameBuffer
{
public:
	void ReserveMemory() override;
	void FreeMemory() override;

	void UpdateFBO() override;
};