#include "Thebe/PhysicsSystem.h"
#include "Thebe/EngineParts/PhysicsObject.h"
#include "Thebe/Math/Graph.h"
#include "Thebe/Math/LineSegment.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------ PhysicsSystem ------------------------------

PhysicsSystem::PhysicsSystem()
{
	this->contactCalculatorArray.push_back(new ContactCalculator<GJKConvexHull, GJKConvexHull>());
}

/*virtual*/ PhysicsSystem::~PhysicsSystem()
{
	for (auto& contactCalculator : this->contactCalculatorArray)
		delete contactCalculator;
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

	// Step the simulation forward until the given delta-time has been consumed.
	while (deltaTimeSeconds > 0.0)
	{
		double timeStepSeconds = THEBE_MIN(deltaTimeSeconds, THEBE_MAX_PHYSICS_TIME_STEP);
		deltaTimeSeconds -= timeStepSeconds;

		// Determine the net force and torque acting on each object.
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->AccumulateForcesAndTorques();
		}

		// Numerically integreate the equations of motion or whatever ODEs are applicable.
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->IntegrateMotionUnconstrained(timeStepSeconds);
		}

		// Detect and gather all collision pairs.
		std::map<RefHandle, Reference<CollisionSystem::Collision>> collisionMap;
		std::vector<Reference<CollisionSystem::Collision>> collisionArray;
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			collisionArray.clear();
			collisionSystem->FindAllCollisions(physicsObject->GetCollisionObject(), collisionArray);
			for (auto& collision : collisionArray)
				if (collisionMap.find(collision->GetHandle()) == collisionMap.end())
					collisionMap.insert(std::pair(collision->GetHandle(), collision));
		}

		if (collisionMap.size() > 0)
		{
			// Go calculate contact points for each collision pair.
			std::unique_ptr<PhysicsCollision> physicsCollisionArray(new PhysicsCollision[collisionMap.size()]);
			int i = 0;
			for (auto& pair : collisionMap)
			{
				const auto& collision = pair.second;
				PhysicsCollision& physicsCollision = physicsCollisionArray.get()[i++];
				bool objectsSet = physicsCollision.SetObjects(collision);
				THEBE_ASSERT(objectsSet);
				bool contactsCalculated = physicsCollision.CalculateContacts(collision, &this->contactCalculatorArray);
				THEBE_ASSERT(contactsCalculated);
			}

			// Go resolve all detected collisions until no collision needs resolving.
			// Hmmm...something tells me that we might loop indefinitely here in some cases.
			uint32_t resolutionCount = 0;
			do
			{
				resolutionCount = 0;
				for (int i = 0; i < (int)collisionMap.size(); i++)
				{
					PhysicsCollision& physicsCollision = physicsCollisionArray.get()[i];
					if (physicsCollision.Resolve())
						resolutionCount++;
				}
			} while (resolutionCount > 0);
		}
	}
}

//------------------------------ PhysicsSystem::PhysicsCollision ------------------------------

PhysicsSystem::PhysicsCollision::PhysicsCollision()
{
	this->objectA = nullptr;
	this->objectB = nullptr;
}

/*virtual*/ PhysicsSystem::PhysicsCollision::~PhysicsCollision()
{
}

bool PhysicsSystem::PhysicsCollision::SetObjects(const CollisionSystem::Collision* collision)
{
	RefHandle handle = (RefHandle)collision->objectA->GetUserData();
	Reference<ReferenceCounted> ref;
	if (!HandleManager::Get()->GetObjectFromHandle(handle, ref))
		return false;

	this->objectA.SafeSet(ref.Get());
	if (!this->objectA.Get())
		return false;

	handle = (RefHandle)collision->objectB->GetUserData();
	if (!HandleManager::Get()->GetObjectFromHandle(handle, ref))
		return false;

	this->objectB.SafeSet(ref.Get());
	if (!this->objectB.Get())
		return false;

	return true;
}

bool PhysicsSystem::PhysicsCollision::CalculateContacts(
						const CollisionSystem::Collision* collision,
						std::vector<ContactCalculatorInterface*>* contactCalculatorArray)
{
	for (auto calculator : *contactCalculatorArray)
		if (calculator->CalculateContacts(collision->objectA, collision->objectB, this->contactArray))
			return true;

	return false;
}

bool PhysicsSystem::PhysicsCollision::Resolve()
{
	// This is where we apply impulses to the physics objects.
	// We might delegate some work here to the physics objects in question.

	// TODO: Write this.

	return true;
}

//------------------------------ PhysicsSystem::ContactCalculatorInterface ------------------------------

/*static*/ void PhysicsSystem::ContactCalculatorInterface::FlipContactNormals(std::vector<Contact>& contactArray)
{
	for (auto& contact : contactArray)
		contact.unitNormal = -contact.unitNormal;
}

//------------------------------ PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull> ------------------------------

/*virtual*/ bool PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull>::CalculateContacts(
												const CollisionObject* objectA,
												const CollisionObject* objectB,
												std::vector<Contact>& contactArray)
{
	// Note that here we have the advantage of knowing that the shapes in question
	// already intersect one another by the GJK algorithm.  Our job here is just to
	// determine all the vertex/face and edge/edge contacts.

	auto hullA = dynamic_cast<const GJKConvexHull*>(objectA->GetShape());
	auto hullB = dynamic_cast<const GJKConvexHull*>(objectB->GetShape());

	if (!hullA || !hullB)
		return false;

	// Look for vertex/face contacts.

	for (int i = 0; i < hullA->hull.GetNumVertices(); i++)
	{
		const Vector3& vertexA = hullA->hull.GetVertex(i);
		if (objectB->PointOnOrBehindAllPlanes(vertexA))
		{
			int j = objectB->FindPlaneNearestToPoint(vertexA);
			THEBE_ASSERT(j >= 0);
			const Plane& planeB = objectB->GetPlaneArray()[j];
			Contact contact;
			contact.unitNormal = planeB.unitNormal;		// Always point from object B to A.
			contact.surfacePoint = planeB.ClosestPointTo(vertexA);
			contactArray.push_back(contact);
		}
	}

	for (int i = 0; i < hullB->hull.GetNumVertices(); i++)
	{
		const Vector3& vertexB = hullB->hull.GetVertex(i);
		if (objectA->PointOnOrBehindAllPlanes(vertexB))
		{
			int j = objectA->FindPlaneNearestToPoint(vertexB);
			THEBE_ASSERT(j >= 0);
			const Plane& planeA = objectA->GetPlaneArray()[j];
			Contact contact;
			contact.unitNormal = -planeA.unitNormal;		// Always point from object B to A.
			contact.surfacePoint = planeA.ClosestPointTo(vertexB);
			contactArray.push_back(contact);
		}
	}

	// Look for edge/edge contacts.

	for (const Graph::UnorderedEdge& edgeA : objectA->GetEdgeSet())
	{
		LineSegment lineSegA;
		lineSegA.point[0] = hullA->hull.GetVertex(edgeA.i);
		lineSegA.point[1] = hullA->hull.GetVertex(edgeA.j);

		for (const Graph::UnorderedEdge& edgeB : objectB->GetEdgeSet())
		{
			LineSegment lineSegB;
			lineSegB.point[0] = hullB->hull.GetVertex(edgeB.i);
			lineSegB.point[1] = hullB->hull.GetVertex(edgeB.j);

			LineSegment shortestConnector;
			if (shortestConnector.SetAsShortestConnector(lineSegA, lineSegB))
			{
				const Vector3& pointA = shortestConnector.point[0];
				const Vector3& pointB = shortestConnector.point[1];

				if (objectA->PointOnOrBehindAllPlanes(pointB) && objectB->PointOnOrBehindAllPlanes(pointA))
				{
					Contact contact;
					contact.surfacePoint = shortestConnector.Lerp(0.5);
					contact.unitNormal = lineSegA.GetDelta().Cross(lineSegB.GetDelta()).Normalized();

					double dot = (contact.surfacePoint - objectB->GetGeometricCenter()).Dot(contact.unitNormal);
					if (dot < 0.0)
						contact.unitNormal = -contact.unitNormal;

					contactArray.push_back(contact);
				}
			}
		}
	}

	return true;
}