#include "Thebe/EngineParts/Camera.h"

using namespace Thebe;

Camera::Camera()
{
}

/*virtual*/ Camera::~Camera()
{
}

/*virtual*/ bool Camera::Setup(void* data)
{
	return true;
}

/*virtual*/ void Camera::Shutdown()
{
}