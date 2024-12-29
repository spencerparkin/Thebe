#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/BoundingVolumeHierarchy.h"
#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/Graph.h"

namespace Thebe
{
	class DynamicLineRenderer;
	class Space;

	/**
	 * 
	 */
	class THEBE_API CollisionObject : public EnginePart, public BVHObject
	{
	public:
		CollisionObject();
		virtual ~CollisionObject();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const override;

		void SetObjectToWorld(const Transform& objectToWorld);
		const Transform& GetObjectToWorld() const;

		void SetUserData(uintptr_t userData);
		uintptr_t GetUserData() const;

		void DebugDraw(DynamicLineRenderer* lineRenderer, UINT& lineOffset) const;

		UINT64 GetFrameWhenLastMoved() const;
		GJKShape* GetShape();
		const GJKShape* GetShape() const;

		void SetDebugColor(const Vector3& color);

		void SetTargetSpace(Space* targetSpace);
		Space* GetTargetSpace();

	private:

		void GenerateVertices(const Vector3& vertexBase, uint32_t axisFlags, std::vector<Vector3>& vertexArray);

		GJKShape* shape;
		UINT64 frameWhenLastMoved;
		mutable Graph graph;
		mutable std::set<Graph::UnorderedEdge, Graph::UnorderedEdge> edgeSet;
		Vector3 color;
		uintptr_t userData;
		Reference<Space> targetSpace;
	};
}