#pragma once

#include "Thebe/Network/JsonClient.h"
#include "Thebe/Math/Vector3.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API DebugRenderClient : public JsonClient
	{
	public:
		DebugRenderClient();
		virtual ~DebugRenderClient();

		virtual bool Setup() override;

		void AddLine(const std::string& lineSetName, const Vector3& pointA, const Vector3& pointB, const Vector3& color);
		void Clear(const std::string& lineSetName);
	};
}