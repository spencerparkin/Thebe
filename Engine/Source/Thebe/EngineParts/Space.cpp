#include "Thebe/EngineParts/Space.h"

using namespace Thebe;

Space::Space()
{
}

/*virtual*/ Space::~Space()
{
}

/*virtual*/ bool Space::Setup(void* data)
{
	return true;
}

/*virtual*/ void Space::Shutdown()
{
}

/*virtual*/ bool Space::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	return false;
}