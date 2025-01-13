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
	this->stationary = false;

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

	if (this->objectSpaceInertiaTensor.Determinant() == 0.0 && !this->stationary)
	{
		// We should never actually do this at run-time.  This should only happen during the asset build.
		if (!this->CalculateRigidBodyCharacteristics([](const Vector3&) -> double { return 1.0; }))
		{
			THEBE_LOG("Failed to calculate object-space inertia tensor.");
			return false;
		}

		// In case the center of mass was not at origin, update our BVH location.
		this->collisionObject->UpdateBVHLocation();

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

	if (JsonHelper::MatrixFromJsonValue(rootValue->GetValue("object_space_inertia_tensor"), this->objectSpaceInertiaTensor))
	{
		if (!this->objectSpaceInertiaTensorInverse.Invert(this->objectSpaceInertiaTensor))
		{
			THEBE_LOG("Failed to invert object space inertia tensor.");
			return false;
		}
	}

	auto stationaryValue = dynamic_cast<const JsonBool*>(rootValue->GetValue("stationary"));
	this->stationary = stationaryValue ? stationaryValue->GetValue() : false;

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
	rootValue->SetValue("stationary", new JsonBool(this->stationary));

	return true;
}

bool RigidBody::CalculateRigidBodyCharacteristics(std::function<double(const Vector3&)> densityFunction /*= [](const Vector3) -> double { return 1.0; }*/)
{
	GJKShape* shape = this->collisionObject->GetShape();
	AxisAlignedBoundingBox objectBoundingBox = shape->GetObjectBoundingBox();
	
	GJKConvexHull::PointContainmentCache pointContainmentCache;
	void* cache = dynamic_cast<GJKConvexHull*>(shape) ? &pointContainmentCache : nullptr;

	this->totalMass = 0.0;
	double voxelExtent = 0.05;
	Vector3 centerOfMass(0.0, 0.0, 0.0);
	objectBoundingBox.Integrate([this, &densityFunction, &centerOfMass, shape, cache](const AxisAlignedBoundingBox& voxel)
		{
			Vector3 voxelCenter = voxel.GetCenter();
			if (shape->ContainsObjectPoint(voxelCenter, cache))
			{
				double voxelMass = densityFunction(voxelCenter) * voxel.GetVolume();
				this->totalMass += voxelMass;
				centerOfMass += voxelMass * voxelCenter;
			}
		}, voxelExtent);
	
	centerOfMass /= this->totalMass;

	// We need the object-space origin to represent the center of mass.
	shape->Shift(-centerOfMass);

	for (uint32_t i = 0; i < 3; i++)
		for (uint32_t j = 0; j < 3; j++)
			this->objectSpaceInertiaTensor.ele[i][j] = 0.0;

	pointContainmentCache.planeArray.clear();
	objectBoundingBox.Integrate([this, &densityFunction, shape, cache](const AxisAlignedBoundingBox& voxel)
		{
			Vector3 voxelCenter = voxel.GetCenter();
			if (shape->ContainsObjectPoint(voxelCenter, cache))
			{
				double voxelMass = densityFunction(voxelCenter) * voxel.GetVolume();

				this->objectSpaceInertiaTensor.ele[0][0] += voxelMass * (voxelCenter.y * voxelCenter.y + voxelCenter.z * voxelCenter.z);
				this->objectSpaceInertiaTensor.ele[0][1] += -voxelMass * voxelCenter.x * voxelCenter.y;
				this->objectSpaceInertiaTensor.ele[0][2] += -voxelMass * voxelCenter.x * voxelCenter.z;
				this->objectSpaceInertiaTensor.ele[1][0] += -voxelMass * voxelCenter.y * voxelCenter.x;
				this->objectSpaceInertiaTensor.ele[1][1] += voxelMass * (voxelCenter.x * voxelCenter.x + voxelCenter.z * voxelCenter.z);
				this->objectSpaceInertiaTensor.ele[1][2] += -voxelMass * voxelCenter.y * voxelCenter.z;
				this->objectSpaceInertiaTensor.ele[2][0] += -voxelMass * voxelCenter.z * voxelCenter.x;
				this->objectSpaceInertiaTensor.ele[2][1] += -voxelMass * voxelCenter.z * voxelCenter.y;
				this->objectSpaceInertiaTensor.ele[2][2] += voxelMass * (voxelCenter.x * voxelCenter.x + voxelCenter.y * voxelCenter.y);
			}
		}, voxelExtent);

	return true;
}

Vector3 RigidBody::GetLinearVelocity() const
{
	if (this->stationary)
		return Vector3(0.0, 0.0, 0.0);

	return this->linearMomentum / this->totalMass;
}

Vector3 RigidBody::GetAngularVelocity() const
{
	if (this->stationary)
		return Vector3(0.0, 0.0, 0.0);

	Matrix3x3 worldSpaceInertiaTensorInverse;
	this->GetWorldSpaceInertiaTensorInverse(worldSpaceInertiaTensorInverse);
	return worldSpaceInertiaTensorInverse * this->angularMomentum;
}

/*virtual*/ Vector3 RigidBody::GetCenterOfMass() const
{
	return this->collisionObject->GetObjectToWorld().translation;
}

/*virtual*/ double RigidBody::GetTotalMass() const
{
	return this->totalMass;
}

const Vector3& RigidBody::GetLinearMomentum() const
{
	return this->linearMomentum;
}

void RigidBody::SetLinearMomentum(const Vector3& linearMomentum)
{
	this->linearMomentum = linearMomentum;
}

const Vector3& RigidBody::GetAngularMomentum() const
{
	return this->angularMomentum;
}

void RigidBody::SetAngularMomentum(const Vector3& angularMomentum)
{
	this->angularMomentum = angularMomentum;
}

void RigidBody::SetStationary(bool stationary)
{
	this->stationary = stationary;
}

bool RigidBody::IsStationary() const
{
	return this->stationary;
}

/*virtual*/ void RigidBody::ZeroMomentum()
{
	this->linearMomentum.SetComponents(0.0, 0.0, 0.0);
	this->angularMomentum.SetComponents(0.0, 0.0, 0.0);
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

	Vector3 linearVelocity = this->GetLinearVelocity();
	Vector3 angularVelocity = this->GetAngularVelocity();

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