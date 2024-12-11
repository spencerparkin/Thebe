#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

MeshInstance::MeshInstance()
{
}

/*virtual*/ MeshInstance::~MeshInstance()
{
}

void MeshInstance::SetMeshPath(std::filesystem::path& meshPath)
{
	this->meshPath = meshPath;
}

/*virtual*/ bool MeshInstance::Setup()
{
	if (!Space::Setup())
		return false;

	if (this->constantsBuffer.Get() || this->pipelineState.Get())
	{
		THEBE_LOG("Mesh instance already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!this->mesh.Get())
	{
		if (this->meshPath.empty())
		{
			THEBE_LOG("No mesh path set for mesh instance.");
			return false;
		}

		if (!graphicsEngine->LoadEnginePartFromFile(this->meshPath, this->mesh))
		{
			THEBE_LOG("Failed to load mesh for mesh instance.");
			return false;
		}
	}

	ID3D12Device* device = graphicsEngine->GetDevice();

	Material* material = this->mesh->GetMaterial();
	Shader* shader = material->GetShader();

	DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
	if (!csuDescriptorHeap->AllocDescriptorSet(1 + material->GetNumTextures(), this->csuDescriptorSet))
	{
		THEBE_LOG("Failed to allocate descriptor set for mesh instance.");
		return false;
	}

	this->constantsBuffer.Set(new ConstantsBuffer());
	this->constantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->constantsBuffer->SetShader(shader);
	this->constantsBuffer->SetName("ConstantsBuffer");
	if (!this->constantsBuffer->Setup())
	{
		THEBE_LOG("Failed to setup constants buffer for mesh instance.");
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
	this->csuDescriptorSet.GetCpuHandle(0, handle);
	if (!this->constantsBuffer->CreateResourceView(handle, device))
		return false;

	for (UINT i = 1; i <= material->GetNumTextures(); i++)
	{
		TextureBuffer* texture = material->GetTextureForRegister(i - 1);
		if (!texture)
		{
			THEBE_LOG("Failed to get texture for register %d.", i - 1);
			return false;
		}

		this->csuDescriptorSet.GetCpuHandle(i, handle);
		if (!texture->CreateResourceView(handle, device))
			return false;
	}

	this->pipelineState = graphicsEngine->GetOrCreatePipelineState(material, this->mesh->GetVertexBuffer());
	if (!this->pipelineState.Get())
	{
		THEBE_LOG("Failed to get PSO.");
		return false;
	}

	return true;
}

/*virtual*/ void MeshInstance::Shutdown()
{
	if (this->constantsBuffer.Get())
	{
		this->constantsBuffer->Shutdown();
		this->constantsBuffer = nullptr;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
		csuDescriptorHeap->FreeDescriptorSet(this->csuDescriptorSet);
	}

	this->pipelineState = nullptr;
}

/*virtual*/ bool MeshInstance::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	const Matrix4x4& cameraToProjMatrix = context->camera->GetCameraToProjectionMatrix();

	Matrix4x4 worldToCameraMatrix;
	context->camera->GetWorldToCameraTransform().GetToMatrix(worldToCameraMatrix);

	Matrix4x4 objectToWorldMatrix;
	this->objectToWorld.GetToMatrix(objectToWorldMatrix);

	Matrix4x4 objectToCameraMatrix = worldToCameraMatrix * objectToWorldMatrix;
	Matrix4x4 objectToProjMatrix = cameraToProjMatrix * objectToCameraMatrix;

	if (this->constantsBuffer->GetParameterType("objToProj") == Shader::Parameter::Type::FLOAT4x4)
		this->constantsBuffer->SetParameter("objToProj", objectToProjMatrix);

	if (this->constantsBuffer->GetParameterType("objToCam") == Shader::Parameter::Type::FLOAT4x4)
		this->constantsBuffer->SetParameter("objToCam", objectToCameraMatrix);

	if (!this->constantsBuffer->UpdateIfNecessary(commandList))
	{
		THEBE_LOG("Failed to update constants buffer for mesh instance.");
		return false;
	}

	Material* material = this->mesh->GetMaterial();
	Shader* shader = material->GetShader();
	IndexBuffer* indexBuffer = this->mesh->GetIndexBuffer();
	VertexBuffer* vertexBuffer = this->mesh->GetVertexBuffer();

	commandList->SetGraphicsRootSignature(shader->GetRootSignature());

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	this->csuDescriptorSet.GetGpuHandle(0, handle);
	commandList->SetGraphicsRootDescriptorTable(0, handle);

	commandList->SetPipelineState(this->pipelineState.Get());
	
	commandList->IASetIndexBuffer(indexBuffer->GetIndexBufferView());
	commandList->IASetVertexBuffers(0, 1, vertexBuffer->GetVertexBufferView());
	commandList->IASetPrimitiveTopology(indexBuffer->GetPrimitiveTopology());

	UINT numIndices = indexBuffer->GetIndexCount();
	commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);

	return true;
}

void MeshInstance::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
}

Mesh* MeshInstance::GetMesh()
{
	return this->mesh;
}

/*virtual*/ bool MeshInstance::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!Space::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto meshInstanceValue = dynamic_cast<const JsonString*>(rootValue->GetValue("mesh_instance"));
	if (!meshInstanceValue)
	{
		THEBE_LOG("No mesh instance specified.");
		return false;
	}

	this->meshPath = meshInstanceValue->GetValue();

	return true;
}

/*virtual*/ bool MeshInstance::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!Space::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	if (this->mesh.Get() && this->meshPath.empty())
		const_cast<MeshInstance*>(this)->meshPath = this->mesh->GetMeshPath();

	if (this->meshPath.empty())
	{
		THEBE_LOG("Can't dump mesh instance if no mesh path set.");
		return false;
	}

	rootValue->SetValue("mesh_instance", new JsonString(this->meshPath.string().c_str()));

	return true;
}