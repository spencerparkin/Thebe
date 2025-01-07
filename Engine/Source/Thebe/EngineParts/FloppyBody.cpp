#include "Thebe/EngineParts/FloppyBody.h"

using namespace Thebe;

FloppyBody::FloppyBody()
{
}

/*virtual*/ FloppyBody::~FloppyBody()
{
}

/*virtual*/ bool FloppyBody::Setup()
{
	if (!PhysicsObject::Setup())
		return false;

	return true;
}

/*virtual*/ void FloppyBody::Shutdown()
{
	PhysicsObject::Shutdown();
}

/*virtual*/ bool FloppyBody::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	return false;
}

/*virtual*/ bool FloppyBody::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	return false;
}

/*virtual*/ void FloppyBody::IntegrateMotionUnconstrained(double timeStepSeconds)
{
}

/*virtual*/ void FloppyBody::DebugDraw(DynamicLineRenderer* lineRenderer) const
{
	// TODO: Render the springs here.
}

/*virtual*/ Vector3 FloppyBody::GetCenterOfMass() const
{
	return Vector3(0.0, 0.0, 0.0);
}

/*virtual*/ double FloppyBody::GetTotalMass() const
{
	return 0.0;
}