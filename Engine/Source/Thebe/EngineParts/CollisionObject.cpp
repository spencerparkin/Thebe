#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//-------------------------------- CollisionObject --------------------------------

CollisionObject::CollisionObject()
{
	this->shape = nullptr;
	this->frameWhenLastMoved = -1;
	this->color.SetComponents(1.0, 1.0, 1.0);
	this->userData = 0;
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
		return false;

	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->shape);
	if (convexHull)
	{
		this->graph.FromPolygohMesh(convexHull->hull);
		this->graph.GenerateEdgeSet(this->edgeSet);
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
		this->shape->SetObjectToWorld(objectToWorld);
		
		this->frameWhenLastMoved = graphicsEngine->GetFrameCount();

		bool updated = this->UpdateBVHLocation();
		THEBE_ASSERT(updated);

		if (this->targetSpace.Get())
			this->targetSpace->SetChildToParentTransform(objectToWorld);
	}
}

const Transform& CollisionObject::GetObjectToWorld() const
{
	return this->shape->GetObjectToWorld();
}

void CollisionObject::SetUserData(uintptr_t userData)
{
	this->userData = userData;
}

uintptr_t CollisionObject::GetUserData() const
{
	return this->userData;
}

UINT64 CollisionObject::GetFrameWhenLastMoved() const
{
	return this->frameWhenLastMoved;
}

GJKShape* CollisionObject::GetShape()
{
	return this->shape;
}

const GJKShape* CollisionObject::GetShape() const
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

	std::vector<Vector3> vertexArray;

	double scale = 1.0;
	auto scaleValue = dynamic_cast<const JsonFloat*>(rootValue->GetValue("scale"));
	if (scaleValue)
		scale = scaleValue->GetValue();

	auto polyhedronValue = dynamic_cast<const JsonString*>(rootValue->GetValue("polyhedron"));
	auto hullVerticesValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("hull_vertices"));

	if (polyhedronValue)
	{
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
	}
	else if (hullVerticesValue)
	{
		for (UINT i = 0; i < hullVerticesValue->GetSize(); i++)
		{
			Vector3 vertex;
			if (!JsonHelper::VectorFromJsonValue(hullVerticesValue->GetValue(i), vertex))
			{
				THEBE_LOG("Failed to get vertex %d.", i);
				return false;
			}

			vertexArray.push_back(vertex);
		}
	}

	if(vertexArray.size() > 0)
	{
		auto convexHull = new GJKConvexHull();
		this->shape = convexHull;
	
		for (Vector3& vertex : vertexArray)
			vertex *= scale;

		if (!convexHull->hull.GenerateConvexHull(vertexArray))
		{
			THEBE_LOG("Failed to generate convex hull from point-cloud.");
			return false;
		}

		convexHull->hull.SimplifyFaces(true);
	}

	if (!this->shape)
	{
		THEBE_LOG("Shape is null.");
		return false;
	}

	Transform objectToWorld;
	if (!JsonHelper::TransformFromJsonValue(rootValue->GetValue("object_to_world"), objectToWorld))
	{
		THEBE_LOG("Failed to get object-to-world transform from the JSON data.");
		return false;
	}

	this->shape->SetObjectToWorld(objectToWorld);

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
		for (const Vector3& vertex : convexHull->hull.GetVertexArray())
			verticesValue->PushValue(JsonHelper::VectorToJsonValue(vertex));
	}

	rootValue->SetValue("object_to_world", JsonHelper::TransformToJsonValue(this->shape->GetObjectToWorld()));

	return true;
}

void CollisionObject::DebugDraw(DynamicLineRenderer* lineRenderer, UINT& lineOffset) const
{
	for (const auto& edge : this->edgeSet)
	{
		const Graph::Node* nodeA = this->graph.GetNode(edge.i);
		const Graph::Node* nodeB = this->graph.GetNode(edge.j);

		Vector3 vertexA = this->shape->GetObjectToWorld().TransformPoint(nodeA->GetVertex());
		Vector3 vertexB = this->shape->GetObjectToWorld().TransformPoint(nodeB->GetVertex());

		lineRenderer->SetLine(lineOffset++, vertexA, vertexB, &this->color, &this->color);
	}
}

void CollisionObject::SetDebugColor(const Vector3& color)
{
	this->color = color;
}

/*virtual*/ AxisAlignedBoundingBox CollisionObject::GetWorldBoundingBox() const
{
	return this->shape->GetWorldBoundingBox();
}

void CollisionObject::SetTargetSpace(Space* targetSpace)
{
	this->targetSpace = targetSpace;
}

Space* CollisionObject::GetTargetSpace()
{
	return this->targetSpace;
}