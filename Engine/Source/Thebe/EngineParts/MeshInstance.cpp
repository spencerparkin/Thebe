#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/RenderTarget.h"
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

	if (!graphicsEngine->LoadEnginePartFromFile("Materials/ShadowMaterial.material", this->shadowMaterial))
	{
		THEBE_LOG("Failed to load shadow material for mesh instance.");
		return false;
	}

	ID3D12Device* device = graphicsEngine->GetDevice();
	Material* material = this->mesh->GetMaterial();
	Shader* shader = material->GetShader();
	Shader* shadowShader = shadowMaterial->GetShader();
	DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();

	// Make descriptors for all textures and 1 constants buffer.
	if (!csuDescriptorHeap->AllocDescriptorSet(1 + material->GetNumTextures(), this->csuDescriptorSet))
	{
		THEBE_LOG("Failed to allocate descriptor set for mesh instance.");
		return false;
	}

	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuShadowDescriptorSet))
	{
		THEBE_LOG("Failed to allocate shadow descriptor set for mesh instance.");
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

	this->shadowConstantsBuffer.Set(new ConstantsBuffer());
	this->shadowConstantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->shadowConstantsBuffer->SetShader(shadowShader);
	this->shadowConstantsBuffer->SetName("ShadowConstantsBuffer");
	if (!this->shadowConstantsBuffer->Setup())
	{
		THEBE_LOG("Fafiled to setup shadow constants buffer for mesh instance.");
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
	this->csuDescriptorSet.GetCpuHandle(0, handle);
	if (!this->constantsBuffer->CreateResourceView(handle, device))
		return false;

	for (UINT i = 0; i < material->GetNumTextures(); i++)
	{
		TextureBuffer* texture = material->GetTextureForRegister(i);
		if (!texture)
		{
			THEBE_LOG("Failed to get texture for register %d.", i);
			return false;
		}

		this->csuDescriptorSet.GetCpuHandle(i + 1 /* skip constants buffer */, handle);
		if (!texture->CreateResourceView(handle, device))
			return false;
	}

	this->csuShadowDescriptorSet.GetCpuHandle(0, handle);
	if (!this->shadowConstantsBuffer->CreateResourceView(handle, device))
		return false;

	this->pipelineState = graphicsEngine->GetOrCreatePipelineState(material, this->mesh->GetVertexBuffer());
	if (!this->pipelineState.Get())
	{
		THEBE_LOG("Failed to get or create PSO.");
		return false;
	}

	this->shadowPipelineState = graphicsEngine->GetOrCreatePipelineState(this->shadowMaterial, this->mesh->GetVertexBuffer());
	if (!shadowPipelineState.Get())
	{
		THEBE_LOG("Failed to get or create shadow PSO.");
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

	if (this->shadowConstantsBuffer.Get())
	{
		this->shadowConstantsBuffer->Shutdown();
		this->shadowConstantsBuffer = nullptr;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
		csuDescriptorHeap->FreeDescriptorSet(this->csuDescriptorSet);
		csuDescriptorHeap->FreeDescriptorSet(this->csuShadowDescriptorSet);
	}

	this->pipelineState = nullptr;
	this->shadowPipelineState = nullptr;
}

/*virtual*/ bool MeshInstance::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!context || !context->camera)
		return false;

	ConstantsBuffer* targetConstantsBuffer = nullptr;

	if (context->renderTarget->GetName() == "SwapChain")
		targetConstantsBuffer = this->constantsBuffer.Get();
	else if (context->renderTarget->GetName() == "ShadowBuffer")
		targetConstantsBuffer = this->shadowConstantsBuffer.Get();

	if (!targetConstantsBuffer)
	{
		THEBE_LOG("Render target context (%s) not recognized.", context->renderTarget->GetName().c_str());
		return false;
	}

	const Matrix4x4& cameraToProjMatrix = context->camera->GetCameraToProjectionMatrix();

	Matrix4x4 worldToCameraMatrix;
	context->camera->GetWorldToCameraTransform().GetToMatrix(worldToCameraMatrix);

	Matrix4x4 objectToWorldMatrix;
	this->objectToWorld.GetToMatrix(objectToWorldMatrix);

	Matrix4x4 objectToCameraMatrix = worldToCameraMatrix * objectToWorldMatrix;
	Matrix4x4 objectToProjMatrix = cameraToProjMatrix * objectToCameraMatrix;
	
	targetConstantsBuffer->SetParameter("objToProj", objectToProjMatrix);
	targetConstantsBuffer->SetParameter("objToCam", objectToCameraMatrix);
	targetConstantsBuffer->SetParameter("objToWorld", objectToWorldMatrix);

	context->camera->SetShaderParameters(targetConstantsBuffer);

	if (context->light)
		context->light->SetShaderParameters(targetConstantsBuffer);

	if (!targetConstantsBuffer->UpdateIfNecessary(commandList))
	{
		THEBE_LOG("Failed to update target constants buffer for mesh instance.");
		return false;
	}

	Shader* targetShader = nullptr;
	ID3D12PipelineState* targetPipelineState = nullptr;
	DescriptorHeap::DescriptorSet* targetDescriptorSet = nullptr;

	if (context->renderTarget->GetName() == "SwapChain")
	{
		Material* material = this->mesh->GetMaterial();
		targetShader = material->GetShader();
		targetPipelineState = this->pipelineState.Get();
		targetDescriptorSet = &this->csuDescriptorSet;
	}
	else if (context->renderTarget->GetName() == "ShadowBuffer")
	{
		targetShader = this->shadowMaterial->GetShader();
		targetPipelineState = this->shadowPipelineState.Get();
		targetDescriptorSet = &this->csuShadowDescriptorSet;
	}

	IndexBuffer* indexBuffer = this->mesh->GetIndexBuffer();
	VertexBuffer* vertexBuffer = this->mesh->GetVertexBuffer();

	commandList->SetGraphicsRootSignature(targetShader->GetRootSignature());

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	targetDescriptorSet->GetGpuHandle(0, handle);
	commandList->SetGraphicsRootDescriptorTable(0, handle);

	commandList->SetPipelineState(targetPipelineState);
	
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