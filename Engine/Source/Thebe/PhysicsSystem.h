#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/CollisionSystem.h"
#include <map>

#define THEBE_MAX_PHYSICS_TIME_STEP		1e-3

namespace Thebe
{
	class PhysicsObject;
	class CollisionSystem;

	/**
	 * This is my attempt to do some basic rigid and floppy body simulations.
	 */
	class THEBE_API PhysicsSystem
	{
	public:
		PhysicsSystem();
		virtual ~PhysicsSystem();

		bool TrackObject(PhysicsObject* physicsObject);
		bool UntrackObject(PhysicsObject* physicsObject);
		void UntrackAllObjects();
		void StepSimulation(double deltaTimeSeconds, CollisionSystem* collisionSystem);

		struct Contact
		{
			Vector3 surfacePoint;		///< This is a point of contact shared between two rigid bodies.
			Vector3 unitNormal;			///< This is the contact normal, always pointing from object B to object A by convention.
		};

		class ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const CollisionObject* objectA, const CollisionObject* objectB, std::vector<Contact>& contactArray) = 0;

			static void FlipContactNormals(std::vector<Contact>& contactArray);
		};

		template<typename ShapeTypeA, typename ShapeTypeB>
		class ContactCalculator : public ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const CollisionObject* objectA, const CollisionObject* objectB, std::vector<Contact>& contactArray) override
			{
				return false;
			}
		};

		template<>
		class ContactCalculator<GJKConvexHull, GJKConvexHull> : public ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const CollisionObject* objectA, const CollisionObject* objectB, std::vector<Contact>& contactArray) override;
		};

		class PhysicsCollision
		{
		public:
			PhysicsCollision();
			virtual ~PhysicsCollision();

			bool SetObjects(const CollisionSystem::Collision* collision);
			bool CalculateContacts(const CollisionSystem::Collision* collision, std::vector<ContactCalculatorInterface*>* contactCalculatorArray);
			bool Resolve();

			Reference<PhysicsObject> objectA;
			Reference<PhysicsObject> objectB;

			std::vector<Contact> contactArray;
		};

	private:

		std::map<RefHandle, Reference<PhysicsObject>> physicsObjectMap;
		std::vector<ContactCalculatorInterface*> contactCalculatorArray;
	};
}