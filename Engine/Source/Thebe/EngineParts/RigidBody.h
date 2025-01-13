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
		virtual void IntegrateMotionUnconstrained(double timeStepSeconds) override;
		virtual void ZeroMomentum() override;
		virtual Vector3 GetCenterOfMass() const override;
		virtual double GetTotalMass() const override;

		void GetWorldSpaceInertiaTensor(Matrix3x3& worldSpaceInertiaTensor) const;
		void GetWorldSpaceInertiaTensorInverse(Matrix3x3& worldSpaceInertiaTensorInverse) const;

		Vector3 GetLinearVelocity() const;
		Vector3 GetAngularVelocity() const;

		const Vector3& GetLinearMomentum() const;
		void SetLinearMomentum(const Vector3& linearMomentum);

		const Vector3& GetAngularMomentum() const;
		void SetAngularMomentum(const Vector3& angularMomentum);

		void SetStationary(bool stationary);
		bool IsStationary() const;

		bool CalculateRigidBodyCharacteristics(std::function<double(const Vector3&)> densityFunction = [](const Vector3) -> double { return 1.0; });

	private:

		// Note that our position (center of mass) and orientation are stored in the collision object.
		Vector3 linearMomentum;
		Vector3 angularMomentum;
		double totalMass;
		Matrix3x3 objectSpaceInertiaTensor;
		Matrix3x3 objectSpaceInertiaTensorInverse;
		bool stationary;
	};
}