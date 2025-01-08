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
	class EventSystem;
	class Event;
	class DynamicLineRenderer;
	class RigidBody;
	class FloppyBody;

	/**
	 * This is my attempt to do some basic rigid and floppy body simulations.
	 * 
	 * I used David Baraff's paper "An Introduction to Physically Based Modeling: Rigid Body Simulation I/II".
	 */
	class THEBE_API PhysicsSystem
	{
	public:
		PhysicsSystem();
		virtual ~PhysicsSystem();

		void Initialize(EventSystem* eventSystem);
		bool TrackObject(PhysicsObject* physicsObject);
		bool UntrackObject(PhysicsObject* physicsObject);
		void UntrackAllObjects();
		void StepSimulation(double deltaTimeSeconds, CollisionSystem* collisionSystem);
		void DebugDraw(DynamicLineRenderer* lineRenderer) const;

		struct THEBE_API Contact
		{
			Reference<PhysicsObject> objectA;
			Reference<PhysicsObject> objectB;
			Vector3 surfacePoint;		///< This is the point of contact shared between the two rigid bodies.
			Vector3 unitNormal;			///< This is the contact normal, always pointing from object B to object A by convention.
		};

		class THEBE_API ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const PhysicsObject* objectA, const PhysicsObject* objectB, std::list<Contact>& contactList) = 0;

			static void FlipContactNormals(std::list<Contact>& contactList);
		};

		template<typename ShapeTypeA, typename ShapeTypeB>
		class THEBE_API ContactCalculator : public ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const PhysicsObject* objectA, const PhysicsObject* objectB, std::list<Contact>& contactList) override
			{
				return false;
			}
		};

		template<>
		class THEBE_API ContactCalculator<GJKConvexHull, GJKConvexHull> : public ContactCalculatorInterface
		{
		public:
			virtual bool CalculateContacts(const PhysicsObject* objectA, const PhysicsObject* objectB, std::list<Contact>& contactList) override;
		};

		class THEBE_API ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact) = 0;
		};

		template<typename ObjectTypeA, typename ObjectTypeB>
		class THEBE_API ContactResolver : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact) override
			{
				return false;
			}
		};

		template<>
		class THEBE_API ContactResolver<RigidBody, RigidBody> : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact) override;
		};

		template<>
		class THEBE_API ContactResolver<RigidBody, FloppyBody> : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact) override;
		};

		void SetGravity(const Vector3& accelerationDueToGravity);
		const Vector3& GetGravity() const;

	private:
		void HandleCollisionObjectEvent(const Event* event);

		/**
		 * Add one or more collision contacts to the given list as a function of the given collision.
		 */
		bool GenerateContacts(const CollisionSystem::Collision* collision, std::list<Contact>& contactList);

		/**
		 * True is returned here if and only if an impulse was applied to prevent interpenetration.
		 */
		bool ResolveContact(Contact& contact);

		std::map<RefHandle, Reference<PhysicsObject>> physicsObjectMap;
		std::vector<ContactCalculatorInterface*> contactCalculatorArray;
		std::vector<ContactResolverInterface*> contactResolverArray;

		Vector3 accelerationDueToGravity;
	};
}