#pragma once

#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/Math/Transform.h"

namespace Thebe
{
	/**
	 * These nodes make up the scene hierarchy.
	 */
	class THEBE_API Space : public RenderObject
	{
	public:
		Space();
		virtual ~Space();

		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;
		virtual void AppendAllChildRenderObjects(std::list<RenderObject*>& renderObjectList) override;

		void UpdateObjectToWorldTransform(const Transform& parentToWorld);
		void SetChildToParentTransform(const Transform& childToParent);
		void AddSubSpace(Space* space);
		void ClearAllSubSpaces();

		std::vector<Reference<Space>>& GetSubSpaceArray();

	protected:
		Transform childToParent;
		Transform objectToWorld;
		std::vector<Reference<Space>> subSpaceArray;
	};
}