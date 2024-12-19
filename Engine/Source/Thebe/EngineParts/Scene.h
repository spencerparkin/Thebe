#pragma once

#include "Thebe/EngineParts/RenderObject.h"

namespace Thebe
{
	class Space;
	class RenderTarget;

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
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context) override;
		virtual void PrepareForRender() override;

		void SetRootSpace(Space* space);

	protected:
		void GatherVisibleRenderObjects(std::list<RenderObject*>& renderObjectList, Camera* camera, RenderTarget* renderTarget);

		Reference<Space> rootSpace;
	};
}