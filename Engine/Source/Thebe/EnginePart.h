#pragma once

#include "Thebe/Reference.h"
#include "JsonValue.h"
#include <filesystem>

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

		virtual bool Setup() = 0;
		virtual void Shutdown() = 0;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath);
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const;

	protected:
		mutable RefHandle graphicsEngineHandle;
	};
}