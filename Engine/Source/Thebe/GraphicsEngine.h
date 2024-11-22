#pragma once

#include "Common.h"
#include "Reference.h"

namespace Thebe
{
	class RenderPass;

	/**
	 * An instance of this class facilitates the rendering of graphics.
	 */
	class THEBE_API GraphicsEngine
	{
	public:
		GraphicsEngine();
		virtual ~GraphicsEngine();

		virtual bool Setup();
		virtual void Shutdown();
		virtual void Render();

	private:
		std::vector<Reference<RenderPass>> renderPassArray;
		// TODO: Own device.
		// TODO: Own the command queue.
	};
}