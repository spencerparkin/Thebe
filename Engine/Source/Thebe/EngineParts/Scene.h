#pragma once

#include "Thebe/EngineParts/RenderObject.h"

namespace Thebe
{
	class Space;

	/**
	 * 
	 */
	class THEBE_API Scene : public RenderObject
	{
	public:
		Scene();
		virtual ~Scene();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;
		virtual void PrepareForRender() override;

		void SetRootSpace(Space* space);

	protected:
		void GatherVisibleRenderObjects(std::list<RenderObject*>& renderObjectList, Camera* camera);

		Reference<Space> rootSpace;
	};
}