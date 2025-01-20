#include "Thebe/EngineParts/FloppyBody.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Log.h"

using namespace Thebe;

FloppyBody::FloppyBody()
{
}

/*virtual*/ FloppyBody::~FloppyBody()
{
}

/*virtual*/ bool FloppyBody::Setup()
{
	if (!PhysicsObject::Setup())
		return false;

	auto convexHull = dynamic_cast<GJKConvexHull*>(this->collisionObject->GetShape());
	if (!convexHull)
	{
		THEBE_LOG("Floppy bodies only support convex hull shapes for now.");
		return false;
	}

	// Note that floppy-bodies are a special case where the local
	// (or object) space of the body is always the same as the world space.
	// In other words, the local-to-world transform is always identity.
	// Also note that while being floppy, the hope is that the collision
	// object will always remain convex, despite any deformations it is
	// undergoing as the simulation unfolds.
	std::vector<Vector3>& vertexArray = convexHull->hull.GetVertexArray();
	for (Vector3& vertex : vertexArray)
		vertex = convexHull->GetObjectToWorld().TransformPoint(vertex);
	this->collisionObject->SetObjectToWorld(Transform::Identity());

	if (this->pointMassArray.size() == 0)
	{
		for (unsigned int i = 0; i < convexHull->hull.GetNumVertices(); i++)
		{
			PointMass pointMass;
			pointMass.offset = i;
			pointMass.mass = 0.2;
			pointMass.currentContactNormal.SetComponents(0.0, 0.0, 0.0);
			pointMass.velocity.SetComponents(0.0, 0.0, 0.0);
			this->pointMassArray.push_back(pointMass);
		}
	}
	else
	{
		for (const PointMass& pointMass : this->pointMassArray)
		{
			if (pointMass.offset >= convexHull->hull.GetNumVertices())
			{
				THEBE_LOG("Offset %d is out of range.", pointMass.offset);
				return false;
			}
		}
	}

	if (this->springArray.size() == 0)
	{
		std::set<Graph::UnorderedEdge, Graph::UnorderedEdge> edgeSet;
		convexHull->GenerateEdgeSet(edgeSet);
		for (const auto& edge : edgeSet)
		{
			Spring spring;
			spring.offset[0] = edge.i;
			spring.offset[1] = edge.j;
			spring.stiffness = 200.0;
			spring.equilibriumLength = 0.0;
			this->springArray.push_back(spring);
		}

		for (unsigned int i = 0; i < convexHull->hull.GetNumVertices(); i++)
		{
			Spring spring;
			spring.offset[0] = i;
			spring.offset[1] = -1;
			spring.stiffness = 200.0;
			spring.equilibriumLength = 0.0;

			// This can create some redundant springs, but...I'm okay with that for now.
			double largestDistance = -1.0;
			for (unsigned int j = 0; j < convexHull->hull.GetNumVertices(); j++)
			{
				double distance = (convexHull->hull.GetVertex(i) - convexHull->hull.GetVertex(j)).Length();
				if (distance > largestDistance)
				{
					largestDistance = distance;
					spring.offset[1] = j;
				}
			}

			this->springArray.push_back(spring);
		}

		for (unsigned int i = 0; i < convexHull->hull.GetNumPolygons(); i++)
		{
			const PolygonMesh::Polygon& polygon = convexHull->hull.GetPolygon(i);

			for (unsigned int j = 0; j < (unsigned int)polygon.vertexArray.size() / 2; j++)
			{
				int k = polygon.Mod(j + (unsigned int)polygon.vertexArray.size() / 2);
				Spring spring;
				spring.offset[0] = polygon.vertexArray[j];
				spring.offset[1] = polygon.vertexArray[k];
				spring.stiffness = 200.0;
				spring.equilibriumLength = 0.0;
				this->springArray.push_back(spring);
			}
		}

		this->SetSpringEquilibriumLengths();
	}
	else
	{
		for (const Spring& spring : this->springArray)
		{
			if (spring.offset[0] >= (unsigned int)this->pointMassArray.size() ||
				spring.offset[1] >= (unsigned int)this->pointMassArray.size())
			{
				THEBE_LOG("Sprint has offset (%d, %d) out of range.", spring.offset[0], spring.offset[1]);
				return false;
			}
		}
	}

	return true;
}

/*virtual*/ void FloppyBody::SetObjectToWorld(const Transform& objectToWorld)
{
	auto convexHull = dynamic_cast<GJKConvexHull*>(this->collisionObject->GetShape());
	if (convexHull)
	{
		Transform appliedTransform;
		appliedTransform.matrix = objectToWorld.matrix;
		appliedTransform.translation = objectToWorld.translation - this->GetCenterOfMass();
		std::vector<Vector3>& vertexArray = convexHull->hull.GetVertexArray();
		for (Vector3& vertex : vertexArray)
			vertex = appliedTransform.TransformPoint(vertex);
	}

	// Tell the collision system that we moved.
	this->collisionObject->SetObjectToWorld(Transform::Identity());
}

/*virtual*/ Transform FloppyBody::GetObjectToWorld() const
{
	Transform objectToWorld;
	objectToWorld.matrix.SetIdentity();
	objectToWorld.translation = this->GetCenterOfMass();
	return objectToWorld;
}

bool FloppyBody::GetWorldVertex(const PointMass& pointMass, Vector3& vertex) const
{
	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->collisionObject->GetShape());
	if (!convexHull)
		return false;

	vertex = convexHull->hull.GetVertex(pointMass.offset);
	return true;
}

bool FloppyBody::GetSpringWorldVertices(const Spring& spring, Vector3& vertexA, Vector3& vertexB) const
{
	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->collisionObject->GetShape());
	if (!convexHull)
		return false;

	vertexA = convexHull->hull.GetVertex(this->pointMassArray[spring.offset[0]].offset);
	vertexB = convexHull->hull.GetVertex(this->pointMassArray[spring.offset[1]].offset);
	return true;
}

void FloppyBody::SetSpringEquilibriumLengths()
{
	for (Spring& spring : this->springArray)
	{
		Vector3 vertexA, vertexB;
		if (this->GetSpringWorldVertices(spring, vertexA, vertexB))
			spring.equilibriumLength = (vertexA - vertexB).Length();
	}
}

/*virtual*/ void FloppyBody::Shutdown()
{
	PhysicsObject::Shutdown();
}

/*virtual*/ bool FloppyBody::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!PhysicsObject::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	this->pointMassArray.clear();
	auto pointMassArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("point_mass_array"));
	if (pointMassArrayValue)
	{
		for (unsigned int i = 0; i < pointMassArrayValue->GetSize(); i++)
		{
			auto pointMassValue = dynamic_cast<const JsonObject*>(pointMassArrayValue->GetValue(i));
			if (!pointMassValue)
			{
				THEBE_LOG("Expected point mass entry %d to be an object.", i);
				return false;
			}

			auto offsetValue = dynamic_cast<const JsonInt*>(pointMassValue->GetValue("offset"));
			if (!offsetValue)
			{
				THEBE_LOG("Expected offset entry for point mass %d.", i);
				return false;
			}

			auto massValue = dynamic_cast<const JsonFloat*>(pointMassValue->GetValue("mass"));
			if (!massValue)
			{
				THEBE_LOG("Expected mass entry for point mass %d.", i);
				return false;
			}

			PointMass pointMass;
			pointMass.offset = (int)offsetValue->GetValue();
			pointMass.mass = massValue->GetValue();
			this->pointMassArray.push_back(pointMass);
		}
	}
	
	this->springArray.clear();
	auto springArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("spring_array"));
	if (springArrayValue)
	{
		for (unsigned int i = 0; i < springArrayValue->GetSize(); i++)
		{
			auto springValue = dynamic_cast<const JsonObject*>(springArrayValue->GetValue(i));
			if (!springValue)
			{
				THEBE_LOG("Expected spring entry %d to be an object.", i);
				return false;
			}

			auto offsetAValue = dynamic_cast<const JsonInt*>(springValue->GetValue("offset_a"));
			auto offsetBValue = dynamic_cast<const JsonInt*>(springValue->GetValue("offset_b"));
			if (!offsetAValue || !offsetBValue)
			{
				THEBE_LOG("Both offsets not found for spring %d.", i);
				return false;
			}

			auto stiffnessValue = dynamic_cast<const JsonFloat*>(springValue->GetValue("stiffness"));
			if (!stiffnessValue)
			{
				THEBE_LOG("No stiffness value found for spring %d.", i);
				return false;
			}

			auto equilibriumLengthValue = dynamic_cast<const JsonFloat*>(springValue->GetValue("equilibrium_length"));
			if (!equilibriumLengthValue)
			{
				THEBE_LOG("No equilibrium length value found for spring %d.", i);
				return false;
			}

			Spring spring;
			spring.offset[0] = (unsigned int)offsetAValue->GetValue();
			spring.offset[1] = (unsigned int)offsetBValue->GetValue();
			spring.stiffness = stiffnessValue->GetValue();
			spring.equilibriumLength = equilibriumLengthValue->GetValue();
			this->springArray.push_back(spring);
		}
	}

	return true;
}

/*virtual*/ bool FloppyBody::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	return false;
}

/*virtual*/ void FloppyBody::AccumulateForcesAndTorques(PhysicsSystem* physicsSystem)
{
	PhysicsObject::AccumulateForcesAndTorques(physicsSystem);

	for (PointMass& pointMass : this->pointMassArray)
		pointMass.totalForce.SetComponents(0.0, 0.0, 0.0);

	// Apply internal forces to each point mass pair connected by a spring.  (i.e., apply Hooke's law to each spring.)
	for (const Spring& spring : this->springArray)
	{
		Vector3 vertexA, vertexB;
		this->GetSpringWorldVertices(spring, vertexA, vertexB);
		Vector3 springVector = vertexB - vertexA;
		double length = springVector.Length();
		Vector3 springForce = (length - spring.equilibriumLength) * spring.stiffness * springVector / length;
		this->pointMassArray[spring.offset[0]].totalForce += springForce;
		this->pointMassArray[spring.offset[1]].totalForce -= springForce;
	}

	// Apply external forces to each point mass.
	Vector3 centerOfMass = this->GetCenterOfMass();
	for (PointMass& pointMass : this->pointMassArray)
	{
		pointMass.totalForce += this->totalForce;
		Vector3 vertex;
		if (this->GetWorldVertex(pointMass, vertex))
			pointMass.totalForce += this->totalTorque.Cross(vertex - centerOfMass);

		// Provide some air resistence here so that we don't jiggle forever.
		static double airResistance = 0.1;
		pointMass.totalForce += -pointMass.velocity * airResistance;

		// If this point-mass is currently in contact with something, apply a friction force.
		double squareLength = pointMass.currentContactNormal.SquareLength();
		if(squareLength > 0.0)
		{
			Vector3 frictionForceDirection = -pointMass.velocity.RejectedFrom(pointMass.currentContactNormal).Normalized();
			static double coeficientOfFriction = 5.0;
			pointMass.totalForce += coeficientOfFriction * frictionForceDirection;
			pointMass.currentContactNormal.SetComponents(0.0, 0.0, 0.0);
		}
	}
}

/*virtual*/ void FloppyBody::IntegrateMotionUnconstrained(double timeStepSeconds)
{
	auto convexHull = dynamic_cast<GJKConvexHull*>(this->collisionObject->GetShape());
	if (!convexHull)
		return;

	std::vector<Vector3>& vertexArray = convexHull->hull.GetVertexArray();

	for (unsigned int i = 0; i < (unsigned int)this->pointMassArray.size(); i++)
	{
		PointMass& pointMass = this->pointMassArray[i];

		Vector3 acceleration = pointMass.totalForce / pointMass.mass;
		pointMass.velocity += acceleration * timeStepSeconds;

		Vector3& vertex = vertexArray[pointMass.offset];
		vertex += pointMass.velocity * timeStepSeconds;
	}

	// We need to do this so that the collision object updates itself within the collision system.
	// The transform doesn't change, but the vertices do.
	this->collisionObject->SetObjectToWorld(Transform::Identity());
}

/*virtual*/ void FloppyBody::DebugDraw(DynamicLineRenderer* lineRenderer) const
{
	Vector3 color(1.0, 1.0, 0.0);
	for (const Spring& spring : this->springArray)
	{
		Vector3 vertexA, vertexB;
		this->GetSpringWorldVertices(spring, vertexA, vertexB);
		lineRenderer->AddLine(vertexA, vertexB, &color, &color);
	}
}

/*virtual*/ Vector3 FloppyBody::GetLinearMotionDirection() const
{
	return Vector3::Zero();
}

/*virtual*/ Vector3 FloppyBody::GetAngularMotionDirection() const
{
	return Vector3::Zero();
}

bool FloppyBody::RespondToCollisionContact(const Plane& contactPlane)
{
	auto convexHull = dynamic_cast<GJKConvexHull*>(this->collisionObject->GetShape());
	if (!convexHull)
		return false;

	constexpr double elasticityFactor = 0.5;

	bool responded = false;
	double planeThickness = 1e-4;
	std::vector<Vector3>& vertexArray = convexHull->hull.GetVertexArray();
	for (PointMass& pointMass : this->pointMassArray)
	{
		Vector3& vertex = vertexArray[pointMass.offset];
		double distance = contactPlane.SignedDistanceTo(vertex);
		if (distance < -planeThickness / 2.0)
		{
			vertex -= contactPlane.unitNormal * distance;

			double dot = pointMass.velocity.Dot(contactPlane.unitNormal);
			if (dot < 0.0)
			{
				pointMass.currentContactNormal = contactPlane.unitNormal;
				Vector3 impulse = -(1.0 + elasticityFactor) * dot * contactPlane.unitNormal;
				pointMass.velocity += impulse;
				responded = true;
			}
		}
	}

	return responded;
}

/*virtual*/ Vector3 FloppyBody::GetCenterOfMass() const
{
	Vector3 centerOfMass(0.0, 0.0, 0.0);

	auto convexHull = dynamic_cast<const GJKConvexHull*>(this->collisionObject->GetShape());
	if (convexHull && this->pointMassArray.size() > 0)
	{
		for (const PointMass& pointMass : this->pointMassArray)
			centerOfMass += pointMass.mass * convexHull->GetWorldVertex(pointMass.offset);

		centerOfMass /= this->GetTotalMass();
	}

	return centerOfMass;
}

/*virtual*/ double FloppyBody::GetTotalMass() const
{
	double totalMass = 0.0;

	for (const PointMass& pointMass : this->pointMassArray)
		totalMass += pointMass.mass;

	return totalMass;
}

/*virtual*/ void FloppyBody::ZeroMomentum()
{
	for (PointMass& pointMass : this->pointMassArray)
		pointMass.velocity.SetComponents(0.0, 0.0, 0.0);
}