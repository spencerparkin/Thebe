#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include <map>

#define THEBE_MAX_PHYSICS_TIME_STEP		1e-3

namespace Thebe
{
	class PhysicsObject;

	/**
	 * 
	 */
	class THEBE_API PhysicsSystem
	{
	public:
		PhysicsSystem();
		virtual ~PhysicsSystem();

		bool TrackObject(PhysicsObject* physicsObject);
		bool UntrackObject(PhysicsObject* physicsObject);
		void UntrackAllObjects();
		void StepSimulation(double deltaTimeSeconds);

	private:
		std::map<RefHandle, Reference<PhysicsObject>> physicsObjectMap;
	};
}