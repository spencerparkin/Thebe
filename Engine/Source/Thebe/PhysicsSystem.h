#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/CollisionSystem.h"
#include "Thebe/ImGuiManager.h"
#include <unordered_map>

#define THEBE_MAX_PHYSICS_TIME_STEP		0.05

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
			virtual bool ResolveContact(Contact& contact, PhysicsSystem* physicsSystem) = 0;
		};

		template<typename ObjectTypeA, typename ObjectTypeB>
		class THEBE_API ContactResolver : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact, PhysicsSystem* physicsSystem) override
			{
				return false;
			}
		};

		template<>
		class THEBE_API ContactResolver<RigidBody, RigidBody> : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact, PhysicsSystem* physicsSystem) override;
		};

		template<>
		class THEBE_API ContactResolver<RigidBody, FloppyBody> : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact, PhysicsSystem* physicsSystem) override;
		};

		template<>
		class THEBE_API ContactResolver<FloppyBody, FloppyBody> : public ContactResolverInterface
		{
		public:
			virtual bool ResolveContact(Contact& contact, PhysicsSystem* physicsSystem) override;
		};

		void SetGravity(const Vector3& accelerationDueToGravity);
		const Vector3& GetGravity() const;

		double GetCoeficientOfRestituation() const;

		void RegisterWithImGuiManager();
		void EnablePhysicsImGuiWindow(bool enable);
		bool ShowingPhysicsImGuiWindow();

	private:
		void HandleCollisionObjectEvent(const Event* event);

		/**
		 * Add one or more collision contacts to the contacts list as a function of the given collision.
		 */
		bool GenerateContacts(const CollisionSystem::Collision* collision);

		/**
		 * True is returned here if and only if an impulse was applied to prevent interpenetration.
		 */
		bool ResolveContact(Contact& contact);

		/**
		 * 
		 */
		void ApplyFriction(Contact& contact);

		void ShowImGuiPhysicsWindow();

		std::unordered_map<RefHandle, Reference<PhysicsObject>> physicsObjectMap;
		std::vector<ContactCalculatorInterface*> contactCalculatorArray;
		std::vector<ContactResolverInterface*> contactResolverArray;

		// These could be declared locally within the StepSimulation function, but
		// we declare them here so that we don't thrash the allocators for each
		// container type.  Rather, we want to re-use that storage each step.
		std::unordered_map<RefHandle, Reference<CollisionSystem::Collision>> collisionMap;
		std::vector<Reference<CollisionSystem::Collision>> collisionArray;
		std::list<Contact> contactList;

		Vector3 accelerationDueToGravity;
		double separationDampingFactor;
		double coeficientOfRestitution;

		int physicsWindowCookie;
	};
}