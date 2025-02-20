#pragma once

#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/Math/Transform.h"
#include <functional>

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

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context) override;
		virtual void AppendAllChildRenderObjects(std::list<RenderObject*>& renderObjectList) override;
		virtual void PrepareForRender() override;
		virtual bool CanBeCollapsed() const;

		void UpdateObjectToWorldTransform(const Transform& parentToWorld);
		void SetChildToParentTransform(const Transform& childToParent);
		const Transform& GetChildToParentTransform() const;
		const Transform& GetObjectToWorldTransoform() const;
		void AddSubSpace(Space* space);
		bool RemoveSubSpace(Space* space);
		void ClearAllSubSpaces();
		void CalcGraphicsMatrices(const Camera* camera, Matrix4x4& objectToProjMatrix, Matrix4x4& objectToCameraMatrix, Matrix4x4& objectToWorldMatrix) const;

		std::vector<Reference<Space>>& GetSubSpaceArray();

		static Space* Factory(const ParseParty::JsonObject* jsonObject);

		Space* FindSpaceByName(const std::string& searchName, Space** parentSpace = nullptr);

		void ForAllSpaces(std::function<void(Space*)> callback);

		void Collapse();
		static void StaticCollapse(Reference<Space>& rootNode);

	protected:
		Transform childToParent;
		Transform objectToWorld;
		std::vector<Reference<Space>> subSpaceArray;
	};
}