#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

Light::Light()
{
	this->lightColor.SetComponents(1.0, 1.0, 1.0);
	this->ambientLightColor.SetComponents(0.03, 0.03, 0.03);
	this->lightIntensity = 5.0;
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

	if (!constantsBuffer->SetParameter("ambientLightColor", this->ambientLightColor))
		return false;

	if (!constantsBuffer->SetParameter("lightIntensity", this->lightIntensity))
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

void Light::SetAmbientLightColor(const Vector3& ambientLightColor)
{
	this->ambientLightColor = ambientLightColor;
}

const Vector3& Light::GetAmbientLightColor() const
{
	return this->ambientLightColor;
}

/*virtual*/ bool Light::SetLightToWorldTransform(const Transform& lightToWorld)
{
	return false;
}

void Light::SetLightIntensity(double lightIntensity)
{
	this->lightIntensity = lightIntensity;
}

double Light::GetLightIntensity() const
{
	return this->lightIntensity;
}