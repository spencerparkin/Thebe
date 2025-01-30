#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/Math/Vector3.h"

namespace Thebe
{
	class PhysicsSystem;
	class DynamicLineRenderer;

	/**
	 * This serves as the base class for any object tracked
	 * by the physics system.
	 */
	class THEBE_API PhysicsObject : public EnginePart
	{
	public:
		PhysicsObject();
		virtual ~PhysicsObject();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;

		/**
		 * Calculate and accumulate all external forces and torques acting
		 * on this physics objects.
		 */
		virtual void AccumulateForcesAndTorques(PhysicsSystem* physicsSystem);

		/**
		 * Move and/or rotate this physics object using numerical integration
		 * of the relevant differential equations.
		 */
		virtual void IntegrateMotionUnconstrained(double timeStepSeconds) = 0;

		/**
		 * Zero any kind of momentum (linear or angular) that the object has.
		 */
		virtual void ZeroMomentum();

		/**
		 * Return the world space center of mass.
		 */
		virtual Vector3 GetCenterOfMass() const = 0;

		/**
		 * Return the mass of this object.
		 */
		virtual double GetTotalMass() const = 0;

		/**
		 * Return a unit-length vector in the direction this object is moving.
		 */
		virtual Vector3 GetLinearMotionDirection() const = 0;

		/**
		 * Return a unit-length vector in the direction of this object's axis of rotation.
		 */
		virtual Vector3 GetAngularMotionDirection() const = 0;

		/**
		 * Provide any debug drawing support you wish.
		 */
		virtual void DebugDraw(DynamicLineRenderer* lineRenderer) const;

		virtual void SetObjectToWorld(const Transform& objectToWorld);
		virtual Transform GetObjectToWorld() const;

		void SetStationary(bool stationary);
		bool IsStationary() const;

		void SetFrozen(bool frozen);
		bool IsFrozen() const;

		void SetCollisionObject(CollisionObject* collisionObject);
		CollisionObject* GetCollisionObject();
		const CollisionObject* GetCollisionObject() const;

		void SetExternalForce(const std::string& name, const Vector3& force);
		Vector3 GetExternalForce(const std::string& name) const;

		void SetExternalTorque(const std::string& name, const Vector3& torque);
		Vector3 GetExternalTorque(const std::string& name) const;

		void SetCollisionObjectPath(const std::filesystem::path& collisionObjectPath);

		struct ContactForce
		{
			Vector3 point;
			Vector3 force;
		};

		void SetExternalContactForce(const std::string& name, const ContactForce& contactForce);
		bool GetExternalContactForce(const std::string& name, ContactForce& contactForce) const;

		void AddTransientContactForce(const ContactForce& contactForce);
		void AddTransientForce(const Vector3& transientForce);

		const Vector3& GetTotalForce() const;
		const Vector3& GetTotalTorque() const;

	protected:

		void ApplyContactForce(const ContactForce& contactForce);

		std::map<std::string, Vector3> externalForceMap;
		std::map<std::string, Vector3> externalTorqueMap;
		std::map<std::string, ContactForce> externalContactForceMap;
		std::list<ContactForce> transientContactForceList;
		std::list<Vector3> transientForceList;

		Vector3 totalForce;
		Vector3 totalTorque;

		Reference<CollisionObject> collisionObject;
		std::filesystem::path collisionObjectPath;

		bool stationary;
		bool frozen;
	};
}