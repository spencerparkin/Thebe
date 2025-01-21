#include "Thebe/PhysicsSystem.h"
#include "Thebe/EventSystem.h"
#include "Thebe/EngineParts/PhysicsObject.h"
#include "Thebe/EngineParts/RigidBody.h"
#include "Thebe/EngineParts/FloppyBody.h"
#include "Thebe/Math/Graph.h"
#include "Thebe/Math/LineSegment.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------ PhysicsSystem ------------------------------

PhysicsSystem::PhysicsSystem()
{
	this->contactCalculatorArray.push_back(new ContactCalculator<GJKConvexHull, GJKConvexHull>());
	this->contactResolverArray.push_back(new ContactResolver<RigidBody, RigidBody>());
	this->contactResolverArray.push_back(new ContactResolver<RigidBody, FloppyBody>());
	this->contactResolverArray.push_back(new ContactResolver<FloppyBody, FloppyBody>());
	this->accelerationDueToGravity.SetComponents(0.0, -9.8, 0.0);
}

/*virtual*/ PhysicsSystem::~PhysicsSystem()
{
	for (auto& contactCalculator : this->contactCalculatorArray)
		delete contactCalculator;

	for (auto& contactResolver : this->contactResolverArray)
		delete contactResolver;
}

void PhysicsSystem::Initialize(EventSystem* eventSystem)
{
	eventSystem->RegisterEventHandler("collision_object", [=](const Event* event) { this->HandleCollisionObjectEvent(event); });
}

void PhysicsSystem::DebugDraw(DynamicLineRenderer* lineRenderer) const
{
	for (const auto& pair : this->physicsObjectMap)
	{
		const PhysicsObject* physicsObject = pair.second;
		physicsObject->DebugDraw(lineRenderer);
	}
}

void PhysicsSystem::HandleCollisionObjectEvent(const Event* event)
{
	auto collisionObjectEvent = dynamic_cast<const CollisionObjectEvent*>(event);
	if (collisionObjectEvent && collisionObjectEvent->what == CollisionObjectEvent::COLLISION_OBJECT_NOT_IN_COLLISION_WORLD)
	{
		RefHandle handle = (RefHandle)collisionObjectEvent->collisionObject->GetPhysicsData();
		Reference<PhysicsObject> physicsObject;
		if (HandleManager::Get()->GetObjectFromHandle(handle, physicsObject))
		{
			physicsObject->ZeroMomentum();
		}
	}
}

void PhysicsSystem::SetGravity(const Vector3& accelerationDueToGravity)
{
	this->accelerationDueToGravity = accelerationDueToGravity;
}

const Vector3& PhysicsSystem::GetGravity() const
{
	return this->accelerationDueToGravity;
}

bool PhysicsSystem::TrackObject(PhysicsObject* physicsObject)
{
	if (!physicsObject)
		return false;

	if (this->physicsObjectMap.find(physicsObject->GetHandle()) != this->physicsObjectMap.end())
	{
		THEBE_LOG("Physics object already tracked.");
		return false;
	}

	this->physicsObjectMap.insert(std::pair(physicsObject->GetHandle(), physicsObject));
	return true;
}

bool PhysicsSystem::UntrackObject(PhysicsObject* physicsObject)
{
	if (!physicsObject)
		return false;

	auto pair = this->physicsObjectMap.find(physicsObject->GetHandle());
	if (pair == this->physicsObjectMap.end())
	{
		THEBE_LOG("Physics object not tracked.  Can't untrack it.");
		return false;
	}

	this->physicsObjectMap.erase(pair);
	return true;
}

void PhysicsSystem::UntrackAllObjects()
{
	this->physicsObjectMap.clear();
}

void PhysicsSystem::StepSimulation(double deltaTimeSeconds, CollisionSystem* collisionSystem)
{
	// If the given time delta is larger than what is reasonable for
	// an animated system, then bail here.  This is one way we can
	// account for being stopped in the debugger, for example.
	if (deltaTimeSeconds >= 1.0)
		return;

	std::map<RefHandle, Reference<CollisionSystem::Collision>> collisionMap;
	std::vector<Reference<CollisionSystem::Collision>> collisionArray;
	std::list<Contact> contactList;

	// Step the simulation forward until the given delta-time has been consumed.
	while (deltaTimeSeconds > 0.0)
	{
		double timeStepSeconds = THEBE_MIN(deltaTimeSeconds, THEBE_MAX_PHYSICS_TIME_STEP);
		deltaTimeSeconds -= timeStepSeconds;

		// Determine the net force and torque acting on each object.
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->AccumulateForcesAndTorques(this);
		}

		// Numerically integreate the equations of motion or whatever ODEs are applicable.
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->IntegrateMotionUnconstrained(timeStepSeconds);
		}

		// Detect and gather all collision pairs.
		collisionMap.clear();
		collisionArray.clear();
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			if (physicsObject->IsStationary())
				continue;

			collisionArray.clear();
			collisionSystem->FindAllCollisions(physicsObject->GetCollisionObject(), collisionArray);
			for (auto& collision : collisionArray)
				if (collisionMap.find(collision->GetHandle()) == collisionMap.end())
					collisionMap.insert(std::pair(collision->GetHandle(), collision));
		}

		// Generate all collision contacts.
		contactList.clear();
		for (auto& pair : collisionMap)
		{
			const auto& collision = pair.second;
			this->GenerateContacts(collision.Get(), contactList);
		}

		// Go process all collision contacts.
		uint32_t resolutionCount = 0;
		uint32_t iterationCount = 0;
		constexpr uint32_t maxIterationCount = 8;
		do
		{
			if (++iterationCount >= maxIterationCount)
				break;

			// Note that Baraff says a more advanced technique involves handling
			// multiple contact points for a single object simultaneously.
			resolutionCount = 0;
			for (Contact& contact : contactList)
				if (this->ResolveContact(contact))
					resolutionCount++;
			
		} while (resolutionCount > 0);

		// Go Separate all the bodies the best we can.
		for (auto& pair : collisionMap)
		{
			auto& collision = pair.second;
			const Vector3& separationDelta = collision->separationDelta;

			RefHandle handleA = (RefHandle)collision->objectA->GetPhysicsData();
			RefHandle handleB = (RefHandle)collision->objectB->GetPhysicsData();

			Reference<PhysicsObject> objectA, objectB;

			if (HandleManager::Get()->GetObjectFromHandle(handleA, objectA) && HandleManager::Get()->GetObjectFromHandle(handleB, objectB))
			{
				if (!objectA->IsStationary() && !objectB->IsStationary())
				{
					Transform objectToWorld = objectA->GetObjectToWorld();
					objectToWorld.translation += separationDelta / 2.0;
					objectA->SetObjectToWorld(objectToWorld);
					objectToWorld = objectB->GetObjectToWorld();
					objectToWorld.translation -= separationDelta / 2.0;
					objectB->SetObjectToWorld(objectToWorld);
				}
			}
		}

		// Lastly, apply some friction forces.
		for (auto& contact : contactList)
		{
			if (!contact.objectA->IsStationary())
			{
				PhysicsObject::TransientFrictionForce transientFrictionForce;
				transientFrictionForce.unitNormal = contact.unitNormal;
				transientFrictionForce.coeficientOfLinearFriction = 0.1;
				transientFrictionForce.coeficientOfAngularFriction = 0.01;
				contact.objectA->AddTransientFrictionForce(transientFrictionForce);
			}

			if (!contact.objectB->IsStationary())
			{
				PhysicsObject::TransientFrictionForce transientFrictionForce;
				transientFrictionForce.unitNormal = -contact.unitNormal;
				transientFrictionForce.coeficientOfLinearFriction = 0.1;
				transientFrictionForce.coeficientOfAngularFriction = 0.01;
				contact.objectA->AddTransientFrictionForce(transientFrictionForce);
			}
		}
	}
}

bool PhysicsSystem::GenerateContacts(const CollisionSystem::Collision* collision, std::list<Contact>& contactList)
{
	Reference<PhysicsObject> objectA, objectB;

	RefHandle handleA = (RefHandle)collision->objectA->GetPhysicsData();
	RefHandle handleB = (RefHandle)collision->objectB->GetPhysicsData();

	if (!HandleManager::Get()->GetObjectFromHandle(handleA, objectA) || !HandleManager::Get()->GetObjectFromHandle(handleB, objectB))
		return false;

	for (auto calculator : this->contactCalculatorArray)
		if (calculator->CalculateContacts(objectA, objectB, contactList))
			return true;

	return true;
}

bool PhysicsSystem::ResolveContact(Contact& contact)
{
	for (auto& contactResolver : this->contactResolverArray)
		if (contactResolver->ResolveContact(contact))
			return true;

	return false;
}

//------------------------------ PhysicsSystem::ContactResolver<RigidBody, RigidBody> ------------------------------

/*virtual*/ bool PhysicsSystem::ContactResolver<RigidBody, RigidBody>::ResolveContact(Contact& contact)
{
	auto rigidBodyA = dynamic_cast<RigidBody*>(contact.objectA.Get());
	auto rigidBodyB = dynamic_cast<RigidBody*>(contact.objectB.Get());
	if (!(rigidBodyA && rigidBodyB))
		return false;
	
	Vector3 contactVectorA = contact.surfacePoint - rigidBodyA->GetCenterOfMass();
	Vector3 contactVectorB = contact.surfacePoint - rigidBodyB->GetCenterOfMass();

	Vector3 velocityA = rigidBodyA->GetLinearVelocity() + rigidBodyA->GetAngularVelocity().Cross(contactVectorA);
	Vector3 velocityB = rigidBodyB->GetLinearVelocity() + rigidBodyB->GetAngularVelocity().Cross(contactVectorB);

	double relativeVelocity = contact.unitNormal.Dot(velocityA - velocityB);
	if (relativeVelocity >= 0.0)
		return false;

	Matrix3x3 worldSpaceInertiaTensorInverseA, worldSpaceInertiaTensorInverseB;

	rigidBodyA->GetWorldSpaceInertiaTensorInverse(worldSpaceInertiaTensorInverseA);
	rigidBodyB->GetWorldSpaceInertiaTensorInverse(worldSpaceInertiaTensorInverseB);

	double denomPartA = 0.0;
	double denomPartB = 0.0;

	if (!rigidBodyA->IsStationary())
		denomPartA = (worldSpaceInertiaTensorInverseA * contactVectorA.Cross(contact.unitNormal)).Cross(contactVectorA).Dot(contact.unitNormal) + 1.0 / rigidBodyA->GetTotalMass();

	if (!rigidBodyB->IsStationary())
		denomPartB = (worldSpaceInertiaTensorInverseB * contactVectorB.Cross(contact.unitNormal)).Cross(contactVectorB).Dot(contact.unitNormal) + 1.0 / rigidBodyB->GetTotalMass();

	double denominator = denomPartA + denomPartB;
	THEBE_ASSERT(denominator != 0.0);

	double coeficientOfRestitution = 0.5;		// TODO: Maybe get this from a setting somewhere?
	double impulseMagnitude = -(1.0 + coeficientOfRestitution) * relativeVelocity / denominator;

	Vector3 impulse = impulseMagnitude * contact.unitNormal;

	Vector3 impulseForceA = impulse;
	Vector3 impulseForceB = -impulse;

	rigidBodyA->SetLinearMomentum(rigidBodyA->GetLinearMomentum() + impulseForceA);
	rigidBodyB->SetLinearMomentum(rigidBodyB->GetLinearMomentum() + impulseForceB);

	Vector3 impulseTorqueA = (contact.surfacePoint - rigidBodyA->GetCenterOfMass()).Cross(impulseForceA);
	Vector3 impulseTorqueB = (contact.surfacePoint - rigidBodyB->GetCenterOfMass()).Cross(impulseForceB);

	rigidBodyA->SetAngularMomentum(rigidBodyA->GetAngularMomentum() + impulseTorqueA);
	rigidBodyB->SetAngularMomentum(rigidBodyB->GetAngularMomentum() + impulseTorqueB);

	return true;
}

//------------------------------ PhysicsSystem::ContactResolver<RigidBody, FloppyBody> ------------------------------

/*virtual*/ bool PhysicsSystem::ContactResolver<RigidBody, FloppyBody>::ResolveContact(Contact& contact)
{
	auto rigidBodyA = dynamic_cast<RigidBody*>(contact.objectA.Get());
	auto rigidBodyB = dynamic_cast<RigidBody*>(contact.objectB.Get());

	auto floppyBodyA = dynamic_cast<FloppyBody*>(contact.objectA.Get());
	auto floppyBodyB = dynamic_cast<FloppyBody*>(contact.objectB.Get());

	RigidBody* rigidBody = nullptr;
	FloppyBody* floppyBody = nullptr;

	Vector3 unitNormalRigidToFloppy;

	if (rigidBodyA && floppyBodyB)
	{
		rigidBody = rigidBodyA;
		floppyBody = floppyBodyB;
		unitNormalRigidToFloppy = -contact.unitNormal;
	}
	else if (floppyBodyA && rigidBodyB)
	{
		rigidBody = rigidBodyB;
		floppyBody = floppyBodyA;
		unitNormalRigidToFloppy = contact.unitNormal;
	}

	if (!rigidBody || !floppyBody)
		return false;

	bool resolved = false;

	if (!rigidBody->IsStationary())
	{
		// TODO: Do rigid body response to floppy body here.
	}

	// TODO: This is just wrong, I think.  We have to handle the collision response
	//       of the floppy body along with the rigid body simultaneously, and it has
	//       to be based on some real physics math derivations.
	Plane contactPlane(contact.surfacePoint, unitNormalRigidToFloppy);
	if (floppyBody->RespondToCollisionContact(contactPlane))
		resolved = true;

	return resolved;
}

/*virtual*/ bool PhysicsSystem::ContactResolver<FloppyBody, FloppyBody>::ResolveContact(Contact& contact)
{
	// TODO: Write this.
	return false;
}

//------------------------------ PhysicsSystem::ContactCalculatorInterface ------------------------------

/*static*/ void PhysicsSystem::ContactCalculatorInterface::FlipContactNormals(std::list<Contact>& contactList)
{
	for (auto& contact : contactList)
		contact.unitNormal = -contact.unitNormal;
}

//------------------------------ PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull> ------------------------------

/*virtual*/ bool PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull>::CalculateContacts(
												const PhysicsObject* objectA,
												const PhysicsObject* objectB,
												std::list<Contact>& contactList)
{
	// Note that here we have the advantage of knowing that the shapes in question
	// already intersect one another by the GJK algorithm.  Our job here is just to
	// determine all the vertex/face and edge/edge contacts.

	const CollisionObject* collisionObjectA = objectA->GetCollisionObject();
	const CollisionObject* collisionObjectB = objectB->GetCollisionObject();

	auto hullA = dynamic_cast<const GJKConvexHull*>(collisionObjectA->GetShape());
	auto hullB = dynamic_cast<const GJKConvexHull*>(collisionObjectB->GetShape());

	if (!hullA || !hullB)
		return false;

	std::vector<Vector3> worldVerticesA;
	worldVerticesA.resize(hullA->hull.GetNumVertices());
	for (int i = 0; i < (int)worldVerticesA.size(); i++)
		worldVerticesA[i] = hullA->GetWorldVertex(i);

	std::vector<Vector3> worldVerticesB;
	worldVerticesB.resize(hullB->hull.GetNumVertices());
	for (int i = 0; i < (int)worldVerticesB.size(); i++)
		worldVerticesB[i] = hullB->GetWorldVertex(i);

	// Look for vertex/face contacts.

	for (int i = 0; i < hullA->hull.GetNumVertices(); i++)
	{
		const Vector3& vertexA = worldVerticesA[i];
		if (collisionObjectB->PointOnOrBehindAllWorldPlanes(vertexA))
		{
			Plane planeB;
			bool found = collisionObjectB->FindWorldPlaneNearestToPoint(vertexA, planeB);
			THEBE_ASSERT(found);
			Contact contact;
			contact.objectA = const_cast<PhysicsObject*>(objectA);
			contact.objectB = const_cast<PhysicsObject*>(objectB);
			contact.unitNormal = planeB.unitNormal;		// Always point from object B to A.
			contact.surfacePoint = planeB.ClosestPointTo(vertexA);
			contactList.push_back(contact);
		}
	}

	for (int i = 0; i < hullB->hull.GetNumVertices(); i++)
	{
		const Vector3& vertexB = worldVerticesB[i];
		if (collisionObjectA->PointOnOrBehindAllWorldPlanes(vertexB))
		{
			Plane planeA;
			bool found = collisionObjectA->FindWorldPlaneNearestToPoint(vertexB, planeA);
			THEBE_ASSERT(found);
			Contact contact;
			contact.objectA = const_cast<PhysicsObject*>(objectA);
			contact.objectB = const_cast<PhysicsObject*>(objectB);
			contact.unitNormal = -planeA.unitNormal;		// Always point from object B to A.
			contact.surfacePoint = planeA.ClosestPointTo(vertexB);
			contactList.push_back(contact);
		}
	}

	// Look for edge/edge contacts.

	for (const Graph::UnorderedEdge& edgeA : collisionObjectA->GetEdgeSet())
	{
		LineSegment lineSegA;
		lineSegA.point[0] = worldVerticesA[edgeA.i];
		lineSegA.point[1] = worldVerticesA[edgeA.j];

		for (const Graph::UnorderedEdge& edgeB : collisionObjectB->GetEdgeSet())
		{
			LineSegment lineSegB;
			lineSegB.point[0] = worldVerticesB[edgeB.i];
			lineSegB.point[1] = worldVerticesB[edgeB.j];

			LineSegment shortestConnector;
			if (shortestConnector.SetAsShortestConnector(lineSegA, lineSegB))
			{
				const Vector3& pointA = shortestConnector.point[0];
				const Vector3& pointB = shortestConnector.point[1];

				if (collisionObjectA->PointOnOrBehindAllWorldPlanes(pointB) && collisionObjectB->PointOnOrBehindAllWorldPlanes(pointA))
				{
					Contact contact;
					contact.objectA = const_cast<PhysicsObject*>(objectA);
					contact.objectB = const_cast<PhysicsObject*>(objectB);
					contact.surfacePoint = shortestConnector.Lerp(0.5);
					contact.unitNormal = lineSegA.GetDelta().Cross(lineSegB.GetDelta()).Normalized();

					double dot = (contact.surfacePoint - collisionObjectB->GetWorldGeometricCenter()).Dot(contact.unitNormal);
					if (dot < 0.0)
						contact.unitNormal = -contact.unitNormal;

					contactList.push_back(contact);
				}
			}
		}
	}

	return true;
}