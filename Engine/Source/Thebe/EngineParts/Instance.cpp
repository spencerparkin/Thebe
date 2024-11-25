#include "Thebe/EngineParts/Instance.h"

using namespace Thebe;

Instance::Instance()
{
}

/*virtual*/ Instance::~Instance()
{
}

/*virtual*/ bool Instance::Setup()
{
	return true;
}

/*virtual*/ void Instance::Shutdown()
{
}

/*virtual*/ bool Instance::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	if (this->renderObject.Get() && !this->renderObject->Render(commandList, camera))
		return false;

	return true;
}