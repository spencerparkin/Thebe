#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/Math/Vector3.h"

namespace Thebe
{
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
		virtual void AccumulateForcesAndTorques();

		/**
		 * Move and/or rotate this physics object using numerical integration
		 * of the relevant differential equations.
		 */
		virtual void IntegrateMotionUnconstrained(double timeStepSeconds) = 0;

		/**
		 * Use the collision system to detect contact with other
		 * physics objects, but do not act on the information; just
		 * collect it now for later use.
		 */
		virtual void DetectCollisions() = 0;

		/**
		 * Act on the information gathered in the @ref DetectCollisions() method.
		 * Calculate and apply impulses.  Prevent bodies from intersecting one another.
		 */
		virtual void ResolveCollisions() = 0;

		void SetCollisionObject(CollisionObject* collisionObject);
		CollisionObject* GetCollisionObject();

		void SetObjectToWorld(const Transform& objectToWorld);
		const Transform& GetObjectToWorld() const;

		void SetExternalForce(const std::string& name, const Vector3& force);
		Vector3 GetExternalForce(const std::string& name) const;

		void SetExternalTorque(const std::string& name, const Vector3& torque);
		Vector3 GetExternalTorque(const std::string& name) const;

		void SetCollisionObjectPath(const std::filesystem::path& collisionObjectPath);

	protected:

		std::map<std::string, Vector3> externalForceMap;
		std::map<std::string, Vector3> externalTorqueMap;

		Vector3 totalForce;
		Vector3 totalTorque;

		Reference<CollisionObject> collisionObject;
		std::filesystem::path collisionObjectPath;
	};
}