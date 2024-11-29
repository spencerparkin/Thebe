#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/Camera.h"

using namespace Thebe;

Scene::Scene()
{
}

/*virtual*/ Scene::~Scene()
{
}

/*virtual*/ bool Scene::Setup()
{
	return true;
}

/*virtual*/ void Scene::Shutdown()
{
}

/*virtual*/ void Scene::PrepareForRender()
{
	if (this->rootSpace)
	{
		Transform parentToWorld;
		parentToWorld.SetIdentity();
		this->rootSpace->UpdateObjectToWorldTransform(parentToWorld);
	}
}

/*virtual*/ bool Scene::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	std::list<RenderObject*> renderObjectList;
	this->GatherVisibleRenderObjects(renderObjectList, camera);

	// This ensures that opaque objects are render before those with some amount of transparency.
	// We might also try to use this mechanism to sort by PSO to reduce PSO switching.
	renderObjectList.sort([](const RenderObject* renderObjectA, const RenderObject* renderObjectB) -> bool
		{
			uint32_t renderOrderA = renderObjectA->GetRenderOrder();
			uint32_t renderOrderB = renderObjectB->GetRenderOrder();
			return renderOrderA < renderOrderB;
		});

	for (RenderObject* renderObject : renderObjectList)
		if (!renderObject->Render(commandList, camera))
			return false;

	return true;
}

void Scene::SetRootSpace(Space* space)
{
	this->rootSpace = space;
}

void Scene::GatherVisibleRenderObjects(std::list<RenderObject*>& renderObjectList, Camera* camera)
{
	renderObjectList.clear();
	if (!this->rootSpace)
		return;

	std::list<RenderObject*> queue;
	queue.push_back(this->rootSpace);
	while (queue.size() > 0)
	{
		RenderObject* renderObject = *queue.begin();
		queue.pop_front();

		uint32_t flags = renderObject->GetFlags();
		if ((flags & THEBE_RENDER_OBJECT_FLAG_VISIBLE) != 0 && camera->CanSee(renderObject))
		{
			renderObjectList.push_back(renderObject);
			renderObject->AppendAllChildRenderObjects(queue);
		}
	}
}