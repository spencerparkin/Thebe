#include "Thebe/PhysicsSystem.h"
#include "Thebe/EngineParts/PhysicsObject.h"
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
	const GJKShape* shapeA = collision->objectA->GetShape();
	const GJKShape* shapeB = collision->objectB->GetShape();

	for (auto calculator : *contactCalculatorArray)
		if (calculator->CalculateContacts(shapeA, shapeB, this->contactArray))
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
		contact.unitSurfaceNormal = -contact.unitSurfaceNormal;
}

//------------------------------ PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull> ------------------------------

/*virtual*/ bool PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull>::CalculateContacts(
												const GJKShape* shapeA,
												const GJKShape* shapeB,
												std::vector<Contact>& contactArray)
{
	auto hullA = dynamic_cast<const GJKConvexHull*>(shapeA);
	auto hullB = dynamic_cast<const GJKConvexHull*>(shapeB);

	if (!hullA || !hullB)
		return false;

	// TODO: Write this.

	return true;
}