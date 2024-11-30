#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

MeshInstance::MeshInstance()
{
}

/*virtual*/ MeshInstance::~MeshInstance()
{
}

/*virtual*/ bool MeshInstance::Setup()
{
	if (this->constantsBuffer.Get() || this->pipelineState.Get())
	{
		THEBE_LOG("Mesh instance already setup.");
		return false;
	}

	if (!this->mesh.Get())
	{
		THEBE_LOG("No mesh set for instantiation.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	Material* material = this->mesh->GetMaterial();
	Shader* shader = material->GetShader();

	this->constantsBuffer.Set(new ConstantsBuffer());
	this->constantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->constantsBuffer->SetShader(shader);
	if (!this->constantsBuffer->Setup())
	{
		THEBE_LOG("Failed to setup constants buffer for mesh instance.");
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

	this->pipelineState = nullptr;
}

/*virtual*/ bool MeshInstance::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (this->constantsBuffer->GetParameterType("objToProj") == Shader::Parameter::Type::FLOAT4x4)
	{
		const Matrix4x4& cameraToProjMatrix = camera->GetCameraToProjectionMatrix();

		Matrix4x4 worldToCameraMatrix;
		camera->GetWorldToCameraTransform().GetToMatrix(worldToCameraMatrix);

		Matrix4x4 objectToWorldMatrix;
		this->objectToWorld.GetToMatrix(objectToWorldMatrix);

		Matrix4x4 objectToProjMatrix = cameraToProjMatrix * worldToCameraMatrix * objectToWorldMatrix;
		this->constantsBuffer->SetParameter("objToProj", objectToProjMatrix);
	}

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

	ID3D12DescriptorHeap* cbvDescriptorHeap = graphicsEngine->GetCbvDescriptorHeap()->GetDescriptorHeap();
	commandList->SetDescriptorHeaps(1, &cbvDescriptorHeap);		// It's unfortunately we can't do this once before all render objects render.  Why not?

	const DescriptorHeap::Descriptor& cbvDescriptor = this->constantsBuffer->GetDescriptor();
	commandList->SetGraphicsRootDescriptorTable(0, cbvDescriptor.gpuHandle);

	commandList->SetPipelineState(this->pipelineState.Get());
	
	commandList->IASetIndexBuffer(indexBuffer->GetIndexBufferView());
	commandList->IASetVertexBuffers(0, 1, vertexBuffer->GetVertexBufferView());
	commandList->IASetPrimitiveTopology(indexBuffer->GetPrimitiveTopology());

	UINT numIndicesPerInstance = indexBuffer->GetIndicesPerInstance();
	UINT numInstances = indexBuffer->GetInstanceCount();
	commandList->DrawIndexedInstanced(numIndicesPerInstance, numInstances, 0, 0, 0);

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