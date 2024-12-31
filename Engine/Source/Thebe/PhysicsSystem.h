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
			Reference<PhysicsObject> objectA;
			Reference<PhysicsObject> objectB;
			Vector3 surfacePoint;		///< This is the point of contact shared between the two rigid bodies.
			Vector3 unitNormal;			///< This is the contact normal, always pointing from object B to object A by convention.
		};

		class ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const PhysicsObject* objectA, const PhysicsObject* objectB, std::list<Contact>& contactList) = 0;

			static void FlipContactNormals(std::list<Contact>& contactList);
		};

		template<typename ShapeTypeA, typename ShapeTypeB>
		class ContactCalculator : public ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const PhysicsObject* objectA, const PhysicsObject* objectB, std::list<Contact>& contactList) override
			{
				return false;
			}
		};

		template<>
		class ContactCalculator<GJKConvexHull, GJKConvexHull> : public ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const PhysicsObject* objectA, const PhysicsObject* objectB, std::list<Contact>& contactList) override;
		};

	private:
		bool GenerateContacts(const CollisionSystem::Collision* collision, std::list<Contact>& contactList);
		bool ResolveContact(const Contact& contact);

		std::map<RefHandle, Reference<PhysicsObject>> physicsObjectMap;
		std::vector<ContactCalculatorInterface*> contactCalculatorArray;
	};
}