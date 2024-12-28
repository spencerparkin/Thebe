#pragma once

#include "Thebe/EngineParts/PhysicsObject.h"
#include "Thebe/Math/Matrix3x3.h"

namespace Thebe
{
	/**
	 * These are solid masses in a convex shape.  We assume uniform
	 * density and a center of mass at object-space origin.
	 */
	class THEBE_API RigidBody : public PhysicsObject
	{
	public:
		RigidBody();
		virtual ~RigidBody();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual void AccumulateForcesAndTorques() override;
		virtual void IntegrateMotionUnconstrained(double timeStepSeconds) override;
		virtual void DetectCollisions() override;
		virtual void ResolveCollisions() override;

		void GetWorldSpaceInertiaTensor(Matrix3x3& worldSpaceInertiaTensor) const;

	private:
		// Note that our position (center of mass) and orientation are stored in the collision object.
		Vector3 linearMomentum;
		Vector3 angularMomentum;
		double totalMass;
		Matrix3x3 objectSpaceInertiaTensor;
	};
}