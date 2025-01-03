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

		/**
		 * These can be useful for debugging algorithm where it's not a good time to draw any lines.
		 * Rather than do that, just record a bunch of lines here, then dump it to file.  Then, later,
		 * some other program can load the lines up so that you can visualize them.
		 */
		class THEBE_API LineSet
		{
			friend class DynamicLineRenderer;

		public:
			LineSet();
			virtual ~LineSet();

			void AddLine(const Vector3& pointA, const Vector3& pointB, const Vector3* colorA = nullptr, const Vector3* colorB = nullptr);
			void Clear();
			void Dump(std::ostream& stream) const;
			void Restore(std::istream& stream);

		private:
			std::vector<Vector3> colorArray;
			std::vector<Vector3> pointArray;
		};

		bool AddLine(const Vector3& pointA, const Vector3& pointB, const Vector3* colorA = nullptr, const Vector3* colorB = nullptr);
		bool AddLineSet(const LineSet& lineSet, int minLine = -1, int maxLine = -1);
		void ResetLines();

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