#include "Thebe/PhysicsSystem.h"
#include "Thebe/EngineParts/PhysicsObject.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/Log.h"

using namespace Thebe;

PhysicsSystem::PhysicsSystem()
{
}

/*virtual*/ PhysicsSystem::~PhysicsSystem()
{
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

void PhysicsSystem::StepSimulation(double deltaTimeSeconds)
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

		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->AccumulateForcesAndTorques();
		}

		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->IntegrateMotionUnconstrained(timeStepSeconds);
		}

		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->DetectCollisions();
		}

		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			physicsObject->ResolveCollisions();
		}

		// Lastly, physics objects don't draw in the world by themselves, but
		// they do drive the object-to-world transforms of stuff that does draw.
		for (auto& pair : this->physicsObjectMap)
		{
			PhysicsObject* physicsObject = pair.second.Get();
			Space* targetSpace = physicsObject->GetTargetSpace();
			if (targetSpace)
				targetSpace->SetChildToParentTransform(physicsObject->GetObjectToWorld());
		}
	}
}