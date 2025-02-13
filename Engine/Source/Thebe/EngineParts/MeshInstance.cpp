#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/CubeMapBuffer.h"
#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/ShadowBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

MeshInstance::MeshInstance()
{
	this->color.SetComponents(1.0, 1.0, 1.0, 1.0);
}

/*virtual*/ MeshInstance::~MeshInstance()
{
}

/*virtual*/ bool MeshInstance::CanBeCollapsed() const
{
	return false;
}

void MeshInstance::SetMeshPath(const std::filesystem::path& meshPath)
{
	this->meshPath = meshPath;
}

void MeshInstance::SetOverrideMaterialPath(const std::filesystem::path& overrideMaterialPath)
{
	this->overrideMaterialPath = overrideMaterialPath;
}

void MeshInstance::SetColor(const Vector4& color)
{
	this->color = color;
}

const Vector4& MeshInstance::GetColor() const
{
	return this->color;
}

/*virtual*/ bool MeshInstance::Setup()
{
	if (!Space::Setup())
		return false;

	if (this->constantsBuffer.Get())
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

	if (this->overrideMaterialPath.string().length() == 0)
		this->material = this->mesh->GetMaterial();
	else
	{
		if (!graphicsEngine->LoadEnginePartFromFile(this->overrideMaterialPath, this->material))
		{
			THEBE_LOG("Failed to load override material: %s", this->overrideMaterialPath.string().c_str());
			return false;
		}
	}

	ID3D12Device* device = graphicsEngine->GetDevice();
	Shader* shader = this->material->GetShader();
	DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();

	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuConstantsBufferDescriptorSet))
		return false;

	this->constantsBuffer.Set(new ConstantsBuffer());
	this->constantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->constantsBuffer->SetShader(shader);
	this->constantsBuffer->SetName("ConstantsBufferForMeshInstance");
	if (!this->constantsBuffer->Setup())
	{
		THEBE_LOG("Failed to setup constants buffer for mesh instance.");
		return false;
	}

	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuShadowConstantsBufferDescriptorSet))
		return false;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
	this->csuConstantsBufferDescriptorSet.GetCpuHandle(0, handle);
	if (!this->constantsBuffer->CreateResourceView(handle, device))
		return false;

	if (this->material->GetCastsShadows())
	{
		if (!graphicsEngine->LoadEnginePartFromFile("Materials/ShadowMaterial.material", this->shadowMaterial))
		{
			THEBE_LOG("Failed to load shadow material for mesh instance.");
			return false;
		}

		Shader* shadowShader = this->shadowMaterial->GetShader();
		this->shadowConstantsBuffer.Set(new ConstantsBuffer());
		this->shadowConstantsBuffer->SetGraphicsEngine(graphicsEngine);
		this->shadowConstantsBuffer->SetShader(shadowShader);
		this->shadowConstantsBuffer->SetName("ShadowConstantsBuffer");
		if (!this->shadowConstantsBuffer->Setup())
		{
			THEBE_LOG("Fafiled to setup shadow constants buffer for mesh instance.");
			return false;
		}
	
		this->csuShadowConstantsBufferDescriptorSet.GetCpuHandle(0, handle);
		if (!this->shadowConstantsBuffer->CreateResourceView(handle, device))
			return false;
	}

	if (shader->GetNumTextureRegisters() > 0)
	{
		if (!csuDescriptorHeap->AllocDescriptorSet(shader->GetNumTextureRegisters(), this->csuMaterialTexturesDescriptorSet))
			return false;

		for (UINT i = 0; i < shader->GetNumTextureRegisters(); i++)
		{
			Buffer* buffer = this->material->GetTextureForRegister(i);
			if (!buffer)
			{
				THEBE_LOG("Failed to get texture for register %d.", i);
				return false;
			}
				
			this->csuMaterialTexturesDescriptorSet.GetCpuHandle(i, handle);
			if (!buffer->CreateResourceView(handle, device))
				return false;
		}
	}

	return true;
}

/*virtual*/ void MeshInstance::Shutdown()
{
	Space::Shutdown();

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
		if (this->csuConstantsBufferDescriptorSet.IsAllocated())
			csuDescriptorHeap->FreeDescriptorSet(this->csuConstantsBufferDescriptorSet);
		if (this->csuShadowConstantsBufferDescriptorSet.IsAllocated())
			csuDescriptorHeap->FreeDescriptorSet(this->csuShadowConstantsBufferDescriptorSet);
		if (this->csuMaterialTexturesDescriptorSet.IsAllocated())
			csuDescriptorHeap->FreeDescriptorSet(this->csuMaterialTexturesDescriptorSet);
	}

	this->material = nullptr;
}

/*virtual*/ bool MeshInstance::RendersToTarget(RenderTarget* renderTarget) const
{
	if (!this->material.Get())
		return false;

	if (renderTarget->GetName() == "ShadowBuffer")
		return this->material->GetCastsShadows();

	return true;
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

	targetConstantsBuffer->SetParameter("color", this->color);

	Matrix4x4 objectToProjMatrix, objectToCameraMatrix, objectToWorldMatrix;
	this->CalcGraphicsMatrices(context->camera, objectToProjMatrix, objectToCameraMatrix, objectToWorldMatrix);
	
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
	DescriptorHeap::DescriptorSet* targetConstantsDescriptorSet = nullptr;
	DescriptorHeap::DescriptorSet* targetTexturesDescriptorSet = nullptr;
	DescriptorHeap::DescriptorSet* targetShadowMapDescriptorSet = nullptr;

	if (context->renderTarget->GetName() == "SwapChain")
	{
		targetPipelineState = graphicsEngine->GetOrCreatePipelineState(this->material, this->mesh->GetVertexBuffer(), this->mesh->GetIndexBuffer(), context->renderTarget);
		targetShader = this->material->GetShader();
		targetConstantsDescriptorSet = &this->csuConstantsBufferDescriptorSet;
		targetTexturesDescriptorSet = &this->csuMaterialTexturesDescriptorSet;

		if (this->material->GetCastsShadows())
		{
			auto shadowBuffer = graphicsEngine->FindRenderTarget<ShadowBuffer>();
			THEBE_ASSERT_FATAL(shadowBuffer != nullptr);
			targetShadowMapDescriptorSet = shadowBuffer->GetShadowMapDescriptorForShader();
		}
	}
	else if (context->renderTarget->GetName() == "ShadowBuffer")
	{
		targetPipelineState = graphicsEngine->GetOrCreatePipelineState(this->shadowMaterial, this->mesh->GetVertexBuffer(), this->mesh->GetIndexBuffer(), context->renderTarget);
		targetShader = this->shadowMaterial->GetShader();
		targetConstantsDescriptorSet = &this->csuShadowConstantsBufferDescriptorSet;
	}
	else
	{
		THEBE_LOG("Render target context (%s) not recognized.", context->renderTarget->GetName().c_str());
		return false;
	}

	commandList->SetGraphicsRootSignature(targetShader->GetRootSignature());
	targetShader->SetRootParameters(commandList, targetConstantsDescriptorSet, targetTexturesDescriptorSet, targetShadowMapDescriptorSet, nullptr);

	commandList->SetPipelineState(targetPipelineState);
	
	IndexBuffer* indexBuffer = this->mesh->GetIndexBuffer();
	VertexBuffer* vertexBuffer = this->mesh->GetVertexBuffer();
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

/*virtual*/ void MeshInstance::PrepareRenderOrder(RenderContext* context) const
{
	this->renderOrder.secondary = 0.0;

	if (this->material.Get() && this->material->RendersTransparency())
		this->renderOrder.primary = THEBE_RENDER_ORDER_ALPHA_BLEND;
	else
		this->renderOrder.primary = THEBE_RENDER_ORDER_OPAQUE;
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

	auto materialOverridePathValue = dynamic_cast<const JsonString*>(rootValue->GetValue("material_override"));
	if (materialOverridePathValue)
		this->SetOverrideMaterialPath(materialOverridePathValue->GetValue());

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

	if (this->overrideMaterialPath.string().length() > 0)
		rootValue->SetValue("material_override", new JsonString(this->overrideMaterialPath.string().c_str()));

	return true;
}