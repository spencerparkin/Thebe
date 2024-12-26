#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/BoundingVolumeHierarchy.h"
#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/Graph.h"

namespace Thebe
{
	class DynamicLineRenderer;

	/**
	 * 
	 */
	// TODO: Derive a class from this one called PhysicsObject?  Have it own mass and an inertia tensor?
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

		void DebugDraw(DynamicLineRenderer* lineRenderer, UINT& lineOffset) const;

		UINT64 GetFrameWhenLastMoved() const;
		GJKShape* GetShape();

#if defined _DEBUG
		void SetDebugColor(const Vector3& color);
#endif //_DEBUG

	private:

		void GenerateVertices(const Vector3& vertexBase, uint32_t axisFlags, std::vector<Vector3>& vertexArray);

		GJKShape* shape;
		UINT64 frameWhenLastMoved;
#if defined _DEBUG
		mutable Graph graph;
		mutable std::set<Graph::UnorderedEdge, Graph::UnorderedEdge> edgeSet;
		Vector3 color;
#endif //_DEBUG
	};
}