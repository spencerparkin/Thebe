#include "Thebe/EngineParts/PhysicsObject.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/PhysicsSystem.h"
#include "Thebe/Log.h"

using namespace Thebe;

PhysicsObject::PhysicsObject()
{
}

/*virtual*/ PhysicsObject::~PhysicsObject()
{
}

/*virtual*/ bool PhysicsObject::Setup()
{
	if (!EnginePart::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	// Don't check the cache, because we always want an instance of the collision object loaded.
	if (!graphicsEngine->LoadEnginePartFromFile(this->collisionObjectPath, this->collisionObject, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
	{
		THEBE_LOG("Failed to load collision object for rigid body.");
		return false;
	}

	if (!graphicsEngine->GetPhysicsSystem()->TrackObject(this))
	{
		THEBE_LOG("Failed to add physics object to physics system.");
		return false;
	}

	this->collisionObject->SetUserData(this->GetHandle());

	return true;
}

/*virtual*/ void PhysicsObject::Shutdown()
{
	if (this->collisionObject.Get())
	{
		this->collisionObject->Shutdown();
		this->collisionObject = nullptr;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
		graphicsEngine->GetPhysicsSystem()->UntrackObject(this);

	EnginePart::Shutdown();
}

/*virtual*/ bool PhysicsObject::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto collisionObjectValue = dynamic_cast<const JsonString*>(rootValue->GetValue("collision_object"));
	if (!collisionObjectValue)
	{
		THEBE_LOG("No collision object specified for physics object.");
		return false;
	}

	this->collisionObjectPath = collisionObjectValue->GetValue();

	return true;
}

/*virtual*/ bool PhysicsObject::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!EnginePart::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("collision_object", new JsonString(this->collisionObjectPath.string()));

	return true;
}

/*virtual*/ void PhysicsObject::AccumulateForcesAndTorques(PhysicsSystem* physicsSystem)
{
	this->totalForce.SetComponents(0.0, 0.0, 0.0);
	this->totalTorque.SetComponents(0.0, 0.0, 0.0);

	for (const auto& pair : this->externalForceMap)
		this->totalForce += pair.second;

	for (const auto& pair : this->externalTorqueMap)
		this->totalTorque += pair.second;

	for (const auto& pair : this->externalContactForceMap)
	{
		const ContactForce& contactForce = pair.second;
		Vector3 vector = this->GetCenterOfMass() - contactForce.point;
		double length = vector.Length();
		double scale = (length == 0.0) ? 0.0 : (1.0 / length);
		if (scale == 0.0 || scale != scale)
			this->totalForce += contactForce.force;
		else
		{
			Vector3 unitVector = vector * scale;
			Vector3 force = unitVector * unitVector.Dot(contactForce.force);
			this->totalForce += force;
			Vector3 torque = (contactForce.force - force).Cross(vector);
			this->totalTorque += torque;
		}
	}

	Vector3 gravityForce = physicsSystem->GetGravity() * this->GetTotalMass();
	this->totalForce += gravityForce;
}

/*virtual*/ void PhysicsObject::ZeroMomentum()
{
}

/*virtual*/ void PhysicsObject::DebugDraw(DynamicLineRenderer* lineRenderer) const
{
}

void PhysicsObject::SetCollisionObject(CollisionObject* collisionObject)
{
	this->collisionObject = collisionObject;
}

CollisionObject* PhysicsObject::GetCollisionObject()
{
	return this->collisionObject;
}

const CollisionObject* PhysicsObject::GetCollisionObject() const
{
	return this->collisionObject;
}

void PhysicsObject::SetCollisionObjectPath(const std::filesystem::path& collisionObjectPath)
{
	this->collisionObjectPath = collisionObjectPath;
}

void PhysicsObject::SetObjectToWorld(const Transform& objectToWorld)
{
	this->collisionObject->SetObjectToWorld(objectToWorld);
}

const Transform& PhysicsObject::GetObjectToWorld() const
{
	return this->collisionObject->GetObjectToWorld();
}

void PhysicsObject::SetExternalForce(const std::string& name, const Vector3& force)
{
	auto pair = this->externalForceMap.find(name);
	if (pair != this->externalForceMap.end())
		this->externalForceMap.erase(pair);

	this->externalForceMap.insert(std::pair(name, force));
}

Vector3 PhysicsObject::GetExternalForce(const std::string& name) const
{
	auto pair = this->externalForceMap.find(name);
	if (pair != this->externalForceMap.end())
		return pair->second;

	return Vector3(0.0, 0.0, 0.0);
}

void PhysicsObject::SetExternalTorque(const std::string& name, const Vector3& torque)
{
	auto pair = this->externalTorqueMap.find(name);
	if (pair != this->externalTorqueMap.end())
		this->externalTorqueMap.erase(pair);

	this->externalTorqueMap.insert(std::pair(name, torque));
}

Vector3 PhysicsObject::GetExternalTorque(const std::string& name) const
{
	auto pair = this->externalTorqueMap.find(name);
	if (pair != this->externalTorqueMap.end())
		return pair->second;

	return Vector3(0.0, 0.0, 0.0);
}

void PhysicsObject::SetExternalContactForce(const std::string& name, const ContactForce& contactForce)
{
	auto pair = this->externalContactForceMap.find(name);
	if (pair != this->externalContactForceMap.end())
		this->externalContactForceMap.erase(pair);

	this->externalContactForceMap.insert(std::pair(name, contactForce));
}

bool PhysicsObject::GetExternalContactForce(const std::string& name, ContactForce& contactForce) const
{
	auto pair = this->externalContactForceMap.find(name);
	if (pair == this->externalContactForceMap.end())
		return false;

	contactForce = pair->second;
	return true;
}