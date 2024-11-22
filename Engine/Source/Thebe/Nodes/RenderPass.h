#pragma once

#include "Node.h"

namespace Thebe
{
	class RenderObject;
	class RenderTarget;
	class Camera;

	/**
	 * Render passes are used to render their configured input (typically a hierarchy of space
	 * nodes) into their configured output (typically a swap-chain node, but could also be a
	 * texture node for purposes of off-screen rendering, or shadow mapping.)
	 * 
	 * Ideally, these would render in parallel on the GPU with other render-passes whenever possible,
	 * only stalling for other render passes if necessary.
	 */
	class THEBE_API RenderPass : public Node
	{
	public:
		RenderPass();
		virtual ~RenderPass();

	private:
		Reference<RenderObject> input;
		Reference<RenderTarget> output;
		Reference<Camera> camera;
		// TODO: Own a graphics command list.
	};
}