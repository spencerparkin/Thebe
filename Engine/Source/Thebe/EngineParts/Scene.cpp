#include "Thebe/EngineParts/Scene.h"

using namespace Thebe;

Scene::Scene()
{
}

/*virtual*/ Scene::~Scene()
{
}

/*virtual*/ bool Scene::Setup(void* data)
{
	return true;
}

/*virtual*/ void Scene::Shutdown()
{
}

/*virtual*/ bool Scene::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	// TODO: Resolve all object-to-world transforms in the scene hierarchy.
	// TODO: Cull render objects that aren't in the view frustum.
	// TODO: Render opaque objects before the transparent ones.

	return true;
}