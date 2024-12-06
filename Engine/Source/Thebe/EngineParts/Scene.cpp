#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/Log.h"

using namespace Thebe;

Scene::Scene()
{
}

/*virtual*/ Scene::~Scene()
{
}

/*virtual*/ bool Scene::Setup()
{
	if (this->rootSpace)
	{
		if (!this->rootSpace->Setup())
			return false;
	}

	return true;
}

/*virtual*/ void Scene::Shutdown()
{
	if (this->rootSpace)
	{
		this->rootSpace->Shutdown();
		this->rootSpace = nullptr;
	}
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

/*virtual*/ bool Scene::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root of JSON to be an object.");
		return false;
	}

	auto spaceValue = dynamic_cast<const JsonObject*>(rootValue->GetValue("root"));
	if (!spaceValue)
	{
		THEBE_LOG("No root space value found.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	this->rootSpace.Set(Space::Factory(spaceValue));
	this->rootSpace->SetGraphicsEngine(graphicsEngine);
	if (!this->rootSpace->LoadConfigurationFromJson(spaceValue, assetPath))
	{
		THEBE_LOG("Root space value failed to load configuration.");
		return false;
	}

	return true;
}

/*virtual*/ bool Scene::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	auto rootValue = new JsonObject();
	jsonValue.reset(rootValue);

	if (this->rootSpace)
	{
		std::unique_ptr<JsonValue> spaceValue;
		if (!this->rootSpace->DumpConfigurationToJson(spaceValue, assetPath))
		{
			THEBE_LOG("Failed to dump root space.");
			return false;
		}

		rootValue->SetValue("root", spaceValue.release());
	}

	return true;
}