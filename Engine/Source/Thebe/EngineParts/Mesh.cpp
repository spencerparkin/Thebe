#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

Mesh::Mesh()
{
}

/*virtual*/ Mesh::~Mesh()
{
}

/*virtual*/ bool Mesh::Setup()
{
	if (this->vertexBuffer.Get() || this->indexBuffer.Get() || this->material.Get())
	{
		THEBE_LOG("Mesh already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!graphicsEngine->LoadEnginePartFromFile(this->indexBufferPath, this->indexBuffer))
	{
		THEBE_LOG("Failed to load index buffer: %s", this->indexBufferPath.string().c_str());
		return false;
	}

	if (!graphicsEngine->LoadEnginePartFromFile(this->vertexBufferPath, this->vertexBuffer))
	{
		THEBE_LOG("Failed to load vertex buffer: %s", this->vertexBufferPath.string().c_str());
		return false;
	}

	if (!graphicsEngine->LoadEnginePartFromFile(this->materialPath, this->material))
	{
		THEBE_LOG("Failed to load material: %s", this->materialPath.string().c_str());
		return false;
	}

	return true;
}

/*virtual*/ void Mesh::Shutdown()
{
	this->vertexBuffer = nullptr;
	this->indexBuffer = nullptr;
	this->material = nullptr;
}

/*virtual*/ bool Mesh::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{
	using namespace ParseParty;

	this->meshPath = relativePath;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected JSON root to be an object.");
		return false;
	}

	auto indexBufferValue = dynamic_cast<const JsonString*>(rootValue->GetValue("index_buffer"));
	if (!indexBufferValue)
	{
		THEBE_LOG("No index buffer given.");
		return false;
	}

	auto vertexBufferValue = dynamic_cast<const JsonString*>(rootValue->GetValue("vertex_buffer"));
	if (!vertexBufferValue)
	{
		THEBE_LOG("No vertex buffer given.");
		return false;
	}

	auto materialValue = dynamic_cast<const JsonString*>(rootValue->GetValue("material"));
	if (!materialValue)
	{
		THEBE_LOG("No material given.");
		return false;
	}

	this->indexBufferPath = indexBufferValue->GetValue();
	this->vertexBufferPath = vertexBufferValue->GetValue();
	this->materialPath = materialValue->GetValue();

	auto nameValue = dynamic_cast<const JsonString*>(rootValue->GetValue("name"));
	if (nameValue)
		this->name = nameValue->GetValue();

	return true;
}

VertexBuffer* Mesh::GetVertexBuffer()
{
	return this->vertexBuffer;
}

IndexBuffer* Mesh::GetIndexBuffer()
{
	return this->indexBuffer;
}

Material* Mesh::GetMaterial()
{
	return this->material;
}

const std::filesystem::path& Mesh::GetMeshPath() const
{
	return this->meshPath;
}