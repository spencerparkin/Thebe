#pragma once

#include "Thebe/EngineParts/RenderTarget.h"

namespace Thebe
{
	/**
	 * These can be used as diffuse textures, normal maps, depth buffers, etc.
	 * They can be configured as render targets for a render pass, or just used
	 * by materials for rendering, or both.  In this latter case, there are
	 * at least two possible applications.  First, for shadow mapping.  And
	 * second, for implimenting a security camera in the scene, for example.
	 * Note that resource barriers will need to be used so that the GPU syncronizes
	 * resource usage correctly.
	 */
	class THEBE_API Texture : public RenderTarget
	{
	public:
		Texture();
		virtual ~Texture();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		// TODO: Own reference to Buffer here?
	};
}