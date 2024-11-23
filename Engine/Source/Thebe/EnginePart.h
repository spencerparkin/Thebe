#pragma once

#include "Thebe/Reference.h"
#include "JsonValue.h"

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

		virtual bool Setup(void* data) = 0;
		virtual void Shutdown() = 0;

		void SetGraphicsEngine(GraphicsEngine* graphicsEngine);
		bool GetGraphicsEngine(Reference<GraphicsEngine>& graphicsEngine);

		virtual bool LoadFromJson(const ParseParty::JsonValue* jsonValue);
		virtual bool DumpToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const;

	protected:
		RefHandle graphicsEngineHandle;
	};
}