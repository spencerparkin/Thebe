#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>

#define THEBE_RENDER_OBJECT_FLAG_VISIBLE			0x00000001
#define THEBE_RENDER_OBJECT_FLAG_CASTS_SHADOW		0x00000002

namespace Thebe
{
	class Camera;
	class Light;

	/**
	 * 
	 */
	class THEBE_API RenderObject : public EnginePart
	{
	public:
		RenderObject();
		virtual ~RenderObject();

		struct RenderContext
		{
			Camera* camera;
			Light* light;
		};

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context);
		virtual void PrepareForRender();
		virtual void AppendAllChildRenderObjects(std::list<RenderObject*>& renderObjectList);
		virtual uint32_t GetRenderOrder() const;

		uint32_t GetFlags() const;
		void SetFlags(uint32_t flags);

	protected:
		uint32_t flags;
	};
}