#pragma once

#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"

namespace Thebe
{
	class Material;

	/**
	 * Being able to dynamically render a set of lines each frame
	 * is an invaluable debugging tool when using a graphcis engine.
	 * Lines might also be drawn as a feature in the application,
	 * but a more common use-case is debugging.  For example, it is
	 * often useful to render a reference frame with axes colored
	 * red, green and blue, or vectors, etc.
	 */
	class THEBE_API DynamicLineRenderer : public RenderObject
	{
	public:
		DynamicLineRenderer();
		virtual ~DynamicLineRenderer();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context);
		virtual bool RendersToTarget(RenderTarget* renderTarget) const;

		bool SetLine(UINT i, const Vector3& pointA, const Vector3& pointB, const Vector3* colorA = nullptr, const Vector3* colorB = nullptr);
		bool GetLine(UINT i, Vector3& pointA, Vector3& pointB, Vector3* colorA = nullptr, Vector3* colorB = nullptr) const;

		void SetLineRenderCount(UINT lineRenderCount);
		UINT GetLineRenderCount() const;

		void SetLineMaxCount(UINT lineMaxCount);
		UINT GetLineMaxCount() const;

	private:
		struct Vertex
		{
			float x, y, z;
			float r, g, b;
		};

		Reference<VertexBuffer> vertexBuffer;
		Reference<Material> material;
		Reference<ConstantsBuffer> constantsBuffer;
		DescriptorHeap::DescriptorSet csuConstantsDescriptorSet;
		UINT lineMaxCount;
		UINT lineRenderCount;
		bool vertexBufferUpdateNeeded;
	};
}