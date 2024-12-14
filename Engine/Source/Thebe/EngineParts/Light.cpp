#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

Light::Light()
{
	this->lightColor.SetComponents(1.0, 1.0, 1.0);
}

/*virtual*/ Light::~Light()
{
}

/*virtual*/ Camera* Light::GetCamera()
{
	return nullptr;
}

/*virtual*/ bool Light::SetShaderParameters(ConstantsBuffer* constantsBuffer)
{
	if (!constantsBuffer->SetParameter("lightColor", this->lightColor))
		return false;

	return true;
}

void Light::SetLightColor(const Vector3& lightColor)
{
	this->lightColor = lightColor;
}

const Vector3& Light::GetLightColor() const
{
	return this->lightColor;
}

/*virtual*/ bool Light::SetLightToWorldTransform(const Transform& lightToWorld)
{
	return false;
}