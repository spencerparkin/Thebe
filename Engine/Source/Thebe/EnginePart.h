#pragma once

#include "Thebe/Reference.h"
#include "JsonValue.h"
#include <filesystem>
#include <d3d12.h>
#include <d3dx12.h>

namespace Thebe
{
	class GraphicsEngine;

	/**
	 * By virtue of being an engine part, you are owned by a
	 * particular graphics engine and have access to that instance.
	 */
	class THEBE_API EnginePart : public ReferenceCounted
	{
	public:
		EnginePart();
		virtual ~EnginePart();

		void SetGraphicsEngine(GraphicsEngine* graphicsEngine);
		bool GetGraphicsEngine(Reference<GraphicsEngine>& graphicsEngine) const;

		virtual bool Setup();
		virtual void Shutdown();
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath);
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const;

		void SetName(const std::string& name);
		const std::string& GetName() const;

	protected:
		mutable RefHandle graphicsEngineHandle;
		std::string name;
	};
}