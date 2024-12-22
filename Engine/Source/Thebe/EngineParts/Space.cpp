#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/Log.h"

using namespace Thebe;

Space::Space()
{
}

/*virtual*/ Space::~Space()
{
}

std::vector<Reference<Space>>& Space::GetSubSpaceArray()
{
	return this->subSpaceArray;
}

/*virtual*/ bool Space::Setup()
{
	for (Space* subSpace : this->subSpaceArray)
		if (!subSpace->Setup())
			return false;
	
	return true;
}

/*virtual*/ void Space::Shutdown()
{
	for (Space* subSpace : this->subSpaceArray)
		subSpace->Shutdown();

	this->subSpaceArray.clear();
}

void Space::CalcGraphicsMatrices(const Camera* camera, Matrix4x4& objectToProjMatrix, Matrix4x4& objectToCameraMatrix, Matrix4x4& objectToWorldMatrix) const
{
	const Matrix4x4& cameraToProjMatrix = camera->GetCameraToProjectionMatrix();

	Matrix4x4 worldToCameraMatrix;
	camera->GetWorldToCameraTransform().GetToMatrix(worldToCameraMatrix);

	this->objectToWorld.GetToMatrix(objectToWorldMatrix);

	objectToCameraMatrix = worldToCameraMatrix * objectToWorldMatrix;
	objectToProjMatrix = cameraToProjMatrix * objectToCameraMatrix;
}

/*virtual*/ bool Space::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
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

	if (!JsonHelper::TransformFromJsonValue(rootValue->GetValue("child_to_parent_transform"), this->childToParent))
	{
		THEBE_LOG("Failed to load child-to-parent transfrom from JSON.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	this->subSpaceArray.clear();
	auto childrenValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("children"));
	if (childrenValue)
	{
		for (unsigned int i = 0; i < childrenValue->GetSize(); i++)
		{
			auto childObject = dynamic_cast<const JsonObject*>(childrenValue->GetValue(i));
			if (!childObject)
			{
				THEBE_LOG("Expected child entry to be a JSON object.");
				return false;
			}

			Reference<Space> space(Factory(childObject));
			space->SetGraphicsEngine(graphicsEngine);
			if (!space->LoadConfigurationFromJson(childObject, assetPath))
				return false;

			this->subSpaceArray.push_back(space);
		}
	}

	return true;
}

/*static*/ Space* Space::Factory(const ParseParty::JsonObject* jsonObject)
{
	if (jsonObject->GetValue("mesh_instance") != nullptr)
		return new MeshInstance();
	
	return new Space();
}

/*virtual*/ bool Space::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!RenderObject::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("child_to_parent_transform", JsonHelper::TransformToJsonValue(this->childToParent));

	auto childrenValue = new JsonArray();
	rootValue->SetValue("children", childrenValue);

	for (const Space* subSpace : this->subSpaceArray)
	{
		std::unique_ptr<JsonValue> childValue;
		if (!subSpace->DumpConfigurationToJson(childValue, assetPath))
			return false;

		childrenValue->PushValue(childValue.release());
	}

	return true;
}

/*virtual*/ void Space::AppendAllChildRenderObjects(std::list<RenderObject*>& renderObjectList)
{
	for (Reference<Space>& space : this->subSpaceArray)
		renderObjectList.push_back(space);
}

/*virtual*/ bool Space::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	return true;
}

void Space::UpdateObjectToWorldTransform(const Transform& parentToWorld)
{
	this->objectToWorld = parentToWorld * this->childToParent;

	for (Reference<Space>& space : this->subSpaceArray)
		space->UpdateObjectToWorldTransform(this->objectToWorld);
}

/*virtual*/ void Space::PrepareForRender()
{
	for (Reference<Space>& space : this->subSpaceArray)
		space->PrepareForRender();
}

void Space::SetChildToParentTransform(const Transform& childToParent)
{
	this->childToParent = childToParent;
}

void Space::AddSubSpace(Space* space)
{
	this->subSpaceArray.push_back(space);
}

void Space::ClearAllSubSpaces()
{
	this->subSpaceArray.clear();
}