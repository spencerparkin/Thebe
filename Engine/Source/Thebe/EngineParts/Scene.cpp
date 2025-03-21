#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/GraphicsEngine.h"
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

		this->rootSpace->PrepareForRender();
	}
}

/*virtual*/ bool Scene::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	std::list<RenderObject*> renderObjectList;
	this->GatherVisibleRenderObjects(renderObjectList, context->camera, context->renderTarget);

	for (auto renderObject : renderObjectList)
		renderObject->PrepareRenderOrder(context);

	// This ensures that opaque objects are render before those with some amount of transparency.
	// We might also try to use this mechanism to sort by PSO to reduce PSO switching.
	renderObjectList.sort([](const RenderObject* renderObjectA, const RenderObject* renderObjectB) -> bool
		{
			const RenderObject::RenderOrder& orderA = renderObjectA->GetRenderOrder();
			const RenderObject::RenderOrder& orderB = renderObjectB->GetRenderOrder();

			if (orderA.primary == orderB.primary)
				return orderA.secondary < orderB.secondary;

			return orderA.primary < orderB.primary;
		});

	for (RenderObject* renderObject : renderObjectList)
		if (!renderObject->Render(commandList, context))
			return false;

	return true;
}

void Scene::SetRootSpace(Space* space)
{
	this->rootSpace = space;
}

Space* Scene::GetRootSpace()
{
	return this->rootSpace;
}

void Scene::GatherVisibleRenderObjects(std::list<RenderObject*>& renderObjectList, Camera* camera, RenderTarget* renderTarget)
{
	renderObjectList.clear();
	
	std::list<RenderObject*> queue;
	if (this->rootSpace.Get())
		queue.push_back(this->rootSpace);
	for (auto renderObject : this->renderObjectArray)
		queue.push_back(renderObject);

	while (queue.size() > 0)
	{
		RenderObject* renderObject = *queue.begin();
		queue.pop_front();

		if (renderObject->IsVisible())
		{
			if (renderObject->RendersToTarget(renderTarget) && camera->CanSee(renderObject))
				renderObjectList.push_back(renderObject);

			renderObject->AppendAllChildRenderObjects(queue);
		}
	}
}

std::vector<Reference<RenderObject>>& Scene::GetRenderObjectArray()
{
	return this->renderObjectArray;
}

/*virtual*/ bool Scene::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!RenderObject::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

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

	if (!RenderObject::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

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