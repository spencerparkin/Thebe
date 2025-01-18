#pragma once

#include "Thebe/Network/JsonServer.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Math/LineSegment.h"

namespace Thebe
{
	/**
	 *
	 */
	class THEBE_API DebugRenderServer : public Thebe::JsonServer
	{
	public:
		DebugRenderServer();
		virtual ~DebugRenderServer();

		void Draw(Thebe::DynamicLineRenderer* lineRenderer) const;

		virtual bool Setup() override;

		virtual void ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply) override;

	private:
		struct Line
		{
			Vector3 color;
			LineSegment line;
		};

		class LineSet : public Thebe::ReferenceCounted
		{
		public:
			LineSet();
			virtual ~LineSet();

			void Draw(Thebe::DynamicLineRenderer* lineRenderer) const;
			void Add(const Line& line);
			void Clear();

		private:
			std::vector<Line> lineArray;
		};

		std::map<std::string, Reference<LineSet>> lineSetMap;
	};
}