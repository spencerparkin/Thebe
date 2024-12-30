#include "Thebe/EngineParts/RigidBody.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

RigidBody::RigidBody()
{
	this->totalMass = 0.0;
	this->linearMomentum.SetComponents(0.0, 0.0, 0.0);
	this->angularMomentum.SetComponents(0.0, 0.0, 0.0);

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			this->objectSpaceInertiaTensor.ele[i][j] = 0.0;
			this->objectSpaceInertiaTensorInverse.ele[i][j] = 0.0;
		}
	}
}

/*virtual*/ RigidBody::~RigidBody()
{
}

/*virtual*/ bool RigidBody::Setup()
{
	if (!PhysicsObject::Setup())
		return false;

	if (this->objectSpaceInertiaTensor.Determinant() == 0.0)
	{
		// We should never actually do this at run-time.  This should only happen during the asset build.
		if (!this->collisionObject->GetShape()->CalculateRigidBodyCharacteristics(this->objectSpaceInertiaTensor, this->totalMass, [](const Vector3&) -> double { return 1.0; }))
		{
			THEBE_LOG("Failed to calculate object-space inertia tensor.");
			return false;
		}

		if (!this->objectSpaceInertiaTensorInverse.Invert(this->objectSpaceInertiaTensor))
		{
			THEBE_LOG("Failed to invert inertia tensor.");
			return false;
		}
	}

	return true;
}

/*virtual*/ void RigidBody::Shutdown()
{
	PhysicsObject::Shutdown();
}

/*virtual*/ bool RigidBody::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!PhysicsObject::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto totalMassValue = dynamic_cast<const JsonFloat*>(rootValue->GetValue("total_mass"));
	if (totalMassValue)
		this->totalMass = totalMassValue->GetValue();

	JsonHelper::MatrixFromJsonValue(rootValue->GetValue("object_space_inertia_tensor"), this->objectSpaceInertiaTensor);

	if (!this->objectSpaceInertiaTensorInverse.Invert(this->objectSpaceInertiaTensor))
	{
		THEBE_LOG("Failed to invert inertia tensor.");
		return false;
	}

	return true;
}

/*virtual*/ bool RigidBody::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!PhysicsObject::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("total_mass", new JsonFloat(this->totalMass));
	rootValue->SetValue("object_space_inertia_tensor", JsonHelper::MatrixToJsonValue(this->objectSpaceInertiaTensor));

	return true;
}

/*virtual*/ void RigidBody::AccumulateForcesAndTorques()
{
	PhysicsObject::AccumulateForcesAndTorques();
}

/*virtual*/ void RigidBody::IntegrateMotionUnconstrained(double timeStepSeconds)
{
	if (this->totalMass == 0.0)
	{
		THEBE_LOG("Can't integrate massless body.");
		return;
	}

	Transform objectToWorld = this->collisionObject->GetObjectToWorld();

	Vector3& position = objectToWorld.translation;
	Matrix3x3& orientation = objectToWorld.matrix;

	Vector3 linearVelocity = this->linearMomentum / this->totalMass;

	Matrix3x3 worldSpaceInertiaTensorInverse;
	this->GetWorldSpaceInertiaTensorInverse(worldSpaceInertiaTensorInverse);

	Vector3 angularVelocity = worldSpaceInertiaTensorInverse * this->angularMomentum;

	Matrix3x3 angularVelocityMatrix;
	angularVelocityMatrix.SetForCrossProduct(angularVelocity);

	// Numerically integrate...
	position += linearVelocity * timeStepSeconds;
	orientation += angularVelocityMatrix * orientation * timeStepSeconds;
	linearMomentum += this->totalForce * timeStepSeconds;
	angularMomentum += this->totalTorque * timeStepSeconds;

	// Don't let numerical round-off error cause our orientation matrix to become non-orthonormal.
	orientation = orientation.Orthonormalized(THEBE_AXIS_FLAG_X);

	this->collisionObject->SetObjectToWorld(objectToWorld);
}

/*virtual*/ void RigidBody::DetectCollisions()
{
	// TODO: Write this.  It will be hard.
}

/*virtual*/ void RigidBody::ResolveCollisions()
{
	// TODO: Write this.  It will be harder!
}

void RigidBody::GetWorldSpaceInertiaTensor(Matrix3x3& worldSpaceInertiaTensor) const
{
	const Matrix3x3& objectToWorldOrientationMatrix = this->collisionObject->GetShape()->GetObjectToWorld().matrix;
	const Matrix3x3& objectToWorldOrientationMatrixInverse = objectToWorldOrientationMatrix.Transposed();

	worldSpaceInertiaTensor = objectToWorldOrientationMatrix * this->objectSpaceInertiaTensor * objectToWorldOrientationMatrixInverse;
}

void RigidBody::GetWorldSpaceInertiaTensorInverse(Matrix3x3& worldSpaceInertiaTensorInverse) const
{
	const Matrix3x3& objectToWorldOrientationMatrix = this->collisionObject->GetShape()->GetObjectToWorld().matrix;
	const Matrix3x3& objectToWorldOrientationMatrixInverse = objectToWorldOrientationMatrix.Transposed();

	worldSpaceInertiaTensorInverse = objectToWorldOrientationMatrix * this->objectSpaceInertiaTensorInverse * objectToWorldOrientationMatrixInverse;
}