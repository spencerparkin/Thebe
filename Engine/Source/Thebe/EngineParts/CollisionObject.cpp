#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//-------------------------------- CollisionObject --------------------------------

CollisionObject::CollisionObject()
{
	this->shape = nullptr;
	this->frameWhenLastMoved = -1;
#if _DEBUG
	this->color.SetComponents(1.0, 1.0, 1.0);
#endif //_DEBUG
}

/*virtual*/ CollisionObject::~CollisionObject()
{
}

/*virtual*/ bool CollisionObject::Setup()
{
	if (!EnginePart::Setup())
		return false;
	
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!graphicsEngine->GetCollisionSystem()->TrackObject(this))
	{
		THEBE_LOG("Failed to add collision object to collision system.");
		return false;
	}

	return true;
}

/*virtual*/ void CollisionObject::Shutdown()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
		graphicsEngine->GetCollisionSystem()->UntrackObject(this);

	EnginePart::Shutdown();
}

void CollisionObject::SetObjectToWorld(const Transform& objectToWorld)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		this->shape->objectToWorld = objectToWorld;
		this->frameWhenLastMoved = graphicsEngine->GetFrameCount();
		bool updated = this->UpdateBVHLocation();
		THEBE_ASSERT(updated);
	}
}

const Transform& CollisionObject::GetObjectToWorld() const
{
	return this->shape->objectToWorld;
}

UINT64 CollisionObject::GetFrameWhenLastMoved() const
{
	return this->frameWhenLastMoved;
}

GJKShape* CollisionObject::GetShape()
{
	return this->shape;
}

/*virtual*/ bool CollisionObject::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto polyhedronValue = dynamic_cast<const JsonString*>(rootValue->GetValue("polyhedron"));
	auto hullVerticesValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("hull_vertices"));

	if (polyhedronValue)
	{
		auto scaleValue = dynamic_cast<const JsonFloat*>(rootValue->GetValue("scale"));
		if (!scaleValue)
		{
			THEBE_LOG("No scale value given when specifying a polyhedron.");
			return false;
		}

		double scale = scaleValue->GetValue();

		auto convexHull = new GJKConvexHull();
		this->shape = convexHull;

		std::vector<Vector3> vertexArray;

		std::string polyhedron = polyhedronValue->GetValue();
		if (polyhedron == "hexadron")
		{
			this->GenerateVertices(Vector3(1.0, 1.0, 1.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
		}
		else if (polyhedron == "icosahedron")
		{
			this->GenerateVertices(Vector3(0.0, 1.0, THEBE_PHI), THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(1.0, THEBE_PHI, 0.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y, vertexArray);
			this->GenerateVertices(Vector3(THEBE_PHI, 0.0, 1.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Z, vertexArray);
		}
		else if (polyhedron == "dodecahedron")
		{
			this->GenerateVertices(Vector3(1.0, 1.0, 1.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(0.0, THEBE_PHI, 1.0 / THEBE_PHI), THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(THEBE_PHI, 1.0 / THEBE_PHI, 0.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y, vertexArray);
			this->GenerateVertices(Vector3(1.0 / THEBE_PHI, 0.0, THEBE_PHI), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Z, vertexArray);
		}
		else if (polyhedron == "tetrahedron")
		{
			// This is not a regular tetrahedron, mind.
			this->GenerateVertices(Vector3(0.0, 1.0, 1.0), THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(1.0, -1.0, 0.0), THEBE_AXIS_FLAG_X, vertexArray);
		}
		else if (polyhedron == "icosidodecahedron")
		{
			this->GenerateVertices(Vector3(THEBE_PHI, 0.0, 0.0), THEBE_AXIS_FLAG_X, vertexArray);
			this->GenerateVertices(Vector3(0.0, THEBE_PHI, 0.0), THEBE_AXIS_FLAG_Y, vertexArray);
			this->GenerateVertices(Vector3(0.0, 0.0, THEBE_PHI), THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(0.5, THEBE_PHI / 2.0, THEBE_PHI * THEBE_PHI / 2.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(THEBE_PHI / 2.0, THEBE_PHI * THEBE_PHI / 2.0, 0.5), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
			this->GenerateVertices(Vector3(THEBE_PHI * THEBE_PHI / 2.0, 0.5, THEBE_PHI / 2.0), THEBE_AXIS_FLAG_X | THEBE_AXIS_FLAG_Y | THEBE_AXIS_FLAG_Z, vertexArray);
		}
		else
		{
			THEBE_LOG("Polyhedron \"%s\" not yet supported.", polyhedron.c_str());
			return false;
		}

		for (Vector3& vertex : vertexArray)
			vertex *= scale;

		if (!convexHull->CalculateFromPointCloud(vertexArray))
		{
			THEBE_LOG("Failed to generate convex hull from point-cloud.");
			return false;
		}
	}
	else if (hullVerticesValue)
	{
		auto convexHull = new GJKConvexHull();
		this->shape = convexHull;

		convexHull->vertexArray.clear();
		for (UINT i = 0; i < hullVerticesValue->GetSize(); i++)
		{
			Vector3 vertex;
			if (!JsonHelper::VectorFromJsonValue(hullVerticesValue->GetValue(i), vertex))
			{
				THEBE_LOG("Failed to get vertex %d.", i);
				return false;
			}

			convexHull->vertexArray.push_back(vertex);
		}
	}

	if (!this->shape)
	{
		THEBE_LOG("Shape is null.");
		return false;
	}

	if (!JsonHelper::TransformFromJsonValue(rootValue->GetValue("object_to_world"), this->shape->objectToWorld))
	{
		THEBE_LOG("Failed to get object-to-world transform from the JSON data.");
		return false;
	}

	return true;
}

void CollisionObject::GenerateVertices(const Vector3& vertexBase, uint32_t axisFlags, std::vector<Vector3>& vertexArray)
{
	std::vector<double> xAxisSignArray = { 1.0 };
	std::vector<double> yAxisSignArray = { 1.0 };
	std::vector<double> zAxisSignArray = { 1.0 };

	if ((axisFlags & THEBE_AXIS_FLAG_X) != 0)
		xAxisSignArray.push_back(-1.0);

	if ((axisFlags & THEBE_AXIS_FLAG_Y) != 0)
		yAxisSignArray.push_back(-1.0);

	if ((axisFlags & THEBE_AXIS_FLAG_Z) != 0)
		zAxisSignArray.push_back(-1.0);

	for (UINT i = 0; i < (UINT)xAxisSignArray.size(); i++)
	{
		for (UINT j = 0; j < (UINT)yAxisSignArray.size(); j++)
		{
			for (UINT k = 0; k < (UINT)zAxisSignArray.size(); k++)
			{
				Vector3 vertex(vertexBase);

				vertex.x *= xAxisSignArray[i];
				vertex.y *= yAxisSignArray[j];
				vertex.z *= zAxisSignArray[k];

				vertexArray.push_back(vertex);
			}
		}
	}
}

/*virtual*/ bool CollisionObject::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!this->shape)
	{
		THEBE_LOG("Shape is null.");
		return false;
	}

	if (!EnginePart::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->shape);
	if (convexHull)
	{
		auto verticesValue = new JsonArray();
		rootValue->SetValue("hull_vertices", verticesValue);
		for (const Vector3& vertex : convexHull->vertexArray)
			verticesValue->PushValue(JsonHelper::VectorToJsonValue(vertex));
	}

	rootValue->SetValue("object_to_world", JsonHelper::TransformToJsonValue(this->shape->objectToWorld));

	return true;
}

void CollisionObject::DebugDraw(DynamicLineRenderer* lineRenderer, UINT& lineOffset) const
{
#if defined _DEBUG
	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->shape);
	if (convexHull)
	{
		if (this->edgeSet.size() == 0)
		{
			PolygonMesh polygonMesh;
			convexHull->GeneratePolygonMesh(polygonMesh);
			polygonMesh.SimplifyFaces(true);
			this->graph.FromPolygohMesh(polygonMesh);
			this->graph.GenerateEdgeSet(this->edgeSet);
		}

		for (const auto& edge : this->edgeSet)
		{
			const Graph::Node* nodeA = this->graph.GetNode(edge.i);
			const Graph::Node* nodeB = this->graph.GetNode(edge.j);

			Vector3 vertexA = this->shape->objectToWorld.TransformPoint(nodeA->GetVertex());
			Vector3 vertexB = this->shape->objectToWorld.TransformPoint(nodeB->GetVertex());

			lineRenderer->SetLine(lineOffset++, vertexA, vertexB, &this->color, &this->color);
		}
	}
#endif //_DEBUG
}

#if defined _DEBUG
void CollisionObject::SetDebugColor(const Vector3& color)
{
	this->color = color;
}
#endif //_DEBUG

/*virtual*/ AxisAlignedBoundingBox CollisionObject::GetWorldBoundingBox() const
{
	return this->shape->GetWorldBoundingBox();
}