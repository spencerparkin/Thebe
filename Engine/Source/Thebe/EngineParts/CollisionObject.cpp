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
	this->physicsData = 0;
}

/*virtual*/ CollisionObject::~CollisionObject()
{
	delete this->shape;
}

/*virtual*/ bool CollisionObject::Setup()
{
	if (!this->shape)
		return false;

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
		convexHull->GenerateEdgeSet(this->edgeSet);
		convexHull->GenerateObjectSpacePlaneArray(this->objectSpacePlaneArray);
	}

	this->objectSpaceGeometricCenter = this->shape->CalcGeometricCenter();

	return true;
}

/*virtual*/ void CollisionObject::Shutdown()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
		graphicsEngine->GetCollisionSystem()->UntrackObject(this);

	this->edgeSet.clear();
	this->objectSpacePlaneArray.clear();

	EnginePart::Shutdown();
}

void CollisionObject::SetObjectToWorld(const Transform& objectToWorld)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		this->shape->SetObjectToWorld(objectToWorld);
		
		this->frameWhenLastMoved = graphicsEngine->GetFrameCount();

		if (this->IsInBVH() && !this->UpdateBVHLocation())
		{
			auto event = new CollisionObjectEvent();
			event->collisionObject = this;
			event->what = CollisionObjectEvent::COLLISION_OBJECT_NOT_IN_COLLISION_WORLD;
			event->SetCategory("collision_object");
			graphicsEngine->GetEventSystem()->SendEvent(event);

			graphicsEngine->GetCollisionSystem()->UntrackObject(this);
		}

		if (this->targetSpace.Get())
		{
			// For now, we're assuming the parent transform is identity.
			this->targetSpace->SetChildToParentTransform(objectToWorld * this->targetSpaceRelativeTransform);
		}
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

void CollisionObject::SetPhysicsData(uintptr_t physicsData)
{
	this->physicsData = physicsData;
}

uintptr_t CollisionObject::GetPhysicsData() const
{
	return this->physicsData;
}

UINT64 CollisionObject::GetFrameWhenLastMoved() const
{
	return this->frameWhenLastMoved;
}

void CollisionObject::SetShape(GJKShape* shape)
{
	delete this->shape;
	this->shape = shape;
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

	double uniformScale = 1.0;
	auto scaleValue = dynamic_cast<const JsonFloat*>(rootValue->GetValue("scale"));
	if (scaleValue)
		uniformScale = scaleValue->GetValue();

	Vector3 nonUniformScale(1.0, 1.0, 1.0);
	JsonHelper::VectorFromJsonValue(rootValue->GetValue("non_uniform_scale"), nonUniformScale);

	auto polyhedronValue = dynamic_cast<const JsonString*>(rootValue->GetValue("polyhedron"));
	auto hullVerticesValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("hull_vertices"));
	auto polygonMeshValue = dynamic_cast<const JsonObject*>(rootValue->GetValue("polygon_mesh"));

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
	else if (polygonMeshValue)
	{
		auto convexHull = new GJKConvexHull();
		this->shape = convexHull;
		if (!convexHull->hull.FromJson(polygonMeshValue))
		{
			THEBE_LOG("Failed to load collision object's polygon mesh.");
			return false;
		}
	}

	if(vertexArray.size() > 0)
	{
		auto convexHull = new GJKConvexHull();
		this->shape = convexHull;
	
		for (Vector3& vertex : vertexArray)
		{
			vertex *= uniformScale;
			vertex *= nonUniformScale;
		}

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
		std::unique_ptr<ParseParty::JsonValue> hullValue;
		if (!convexHull->hull.ToJson(hullValue))
			return false;

		rootValue->SetValue("polygon_mesh", hullValue.release());
	}

	rootValue->SetValue("object_to_world", JsonHelper::TransformToJsonValue(this->shape->GetObjectToWorld()));

	return true;
}

/*virtual*/ bool CollisionObject::RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const
{
	if (!this->shape)
		return false;

	return this->shape->RayCast(ray, alpha, unitSurfaceNormal);
}

void CollisionObject::DebugDraw(DynamicLineRenderer* lineRenderer) const
{
	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->shape);
	if (convexHull)
	{
		for (const auto& edge : this->edgeSet)
		{
			Vector3 vertexA = this->shape->GetObjectToWorld().TransformPoint(convexHull->hull.GetVertex(edge.i));
			Vector3 vertexB = this->shape->GetObjectToWorld().TransformPoint(convexHull->hull.GetVertex(edge.j));

			lineRenderer->AddLine(vertexA, vertexB, &this->color, &this->color);
		}
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

void CollisionObject::SetTargetSpace(Space* targetSpace, const Transform& targetSpaceRelativeTransform)
{
	this->targetSpace = targetSpace;
	this->targetSpaceRelativeTransform = targetSpaceRelativeTransform;
}

Space* CollisionObject::GetTargetSpace(Transform* targetSpaceRelativeTransform /*= nullptr*/)
{
	if (targetSpaceRelativeTransform)
		*targetSpaceRelativeTransform = this->targetSpaceRelativeTransform;

	return this->targetSpace;
}

const std::set<Graph::UnorderedEdge, Graph::UnorderedEdge>& CollisionObject::GetEdgeSet() const
{
	return this->edgeSet;
}

const std::vector<Plane>& CollisionObject::GetObjectSpacePlaneArray() const
{
	return this->objectSpacePlaneArray;
}

Vector3 CollisionObject::GetWorldGeometricCenter() const
{
	return this->GetObjectToWorld().TransformPoint(this->objectSpaceGeometricCenter);
}

bool CollisionObject::PointOnOrBehindAllWorldPlanes(const Vector3& point) const
{
	for (const Plane& objectPlane : this->objectSpacePlaneArray)
	{
		Plane worldPlane = this->GetObjectToWorld().TransformPlane(objectPlane);
		if (worldPlane.GetSide(point) == Plane::FRONT)
			return false;
	}

	return true;
}

bool CollisionObject::FindWorldPlaneNearestToPoint(const Vector3& point, Plane& foundWorldPlane) const
{
	double smallestDistance = std::numeric_limits<double>::max();

	for (int i = 0; i < (int)this->objectSpacePlaneArray.size(); i++)
	{
		const Plane& objectPlane = this->objectSpacePlaneArray[i];
		Plane worldPlane = this->GetObjectToWorld().TransformPlane(objectPlane);
		double distance = ::fabs(worldPlane.SignedDistanceTo(point));
		if (distance < smallestDistance)
		{
			smallestDistance = distance;
			foundWorldPlane = worldPlane;
		}
	}

	return smallestDistance != std::numeric_limits<double>::max();
}

//----------------------------------- CollisionObjectEvent -----------------------------------

CollisionObjectEvent::CollisionObjectEvent()
{
	this->what = What::UNKNOWN;
}

/*virtual*/ CollisionObjectEvent::~CollisionObjectEvent()
{
}