#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

DynamicLineRenderer::DynamicLineRenderer()
{
	this->lineMaxCount = 0;
	this->lineRenderCount = 0;
	this->vertexBufferUpdateNeeded = false;
}

/*virtual*/ DynamicLineRenderer::~DynamicLineRenderer()
{
}

/*virtual*/ bool DynamicLineRenderer::Setup()
{
	if (this->vertexBuffer.Get() || this->material.Get())
	{
		THEBE_LOG("Dynamic line renderer already setup.");
		return false;
	}

	if (this->lineMaxCount == 0)
	{
		THEBE_LOG("Max line count is zero.");
		return false;
	}

	if (!RenderObject::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	this->vertexBuffer.Set(new VertexBuffer());
	this->vertexBuffer->SetGraphicsEngine(graphicsEngine);
	this->vertexBuffer->SetBufferType(VertexBuffer::DYNAMIC);
	this->vertexBuffer->SetStride(sizeof(Vertex));
	this->vertexBuffer->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	D3D12_RESOURCE_DESC& bufferDesc = this->vertexBuffer->GetResourceDesc();
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = this->lineMaxCount * 2 * sizeof(Vertex);
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;

	std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescArray = this->vertexBuffer->GetElementDescArray();
	elementDescArray.resize(2);
	elementDescArray[0].AlignedByteOffset = 0;
	elementDescArray[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elementDescArray[0].InputSlot = 0;
	elementDescArray[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elementDescArray[0].InstanceDataStepRate = 0;
	elementDescArray[0].SemanticIndex = 0;
	elementDescArray[0].SemanticName = "POSITION";
	elementDescArray[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elementDescArray[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elementDescArray[1].InputSlot = 0;
	elementDescArray[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elementDescArray[1].InstanceDataStepRate = 0;
	elementDescArray[1].SemanticIndex = 0;
	elementDescArray[1].SemanticName = "COLOR";

	std::vector<UINT8>& originalBuffer = this->vertexBuffer->GetOriginalBuffer();
	originalBuffer.resize(bufferDesc.Width);
	::memset(originalBuffer.data(), 0, originalBuffer.size());

	if (!this->vertexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup vertex buffer for dynamic line renderer.");
		return false;
	}

	if (!graphicsEngine->LoadEnginePartFromFile(R"(Materials\LineMaterial.material)", this->material))
	{
		THEBE_LOG("Failed to load line material.");
		return false;
	}

	this->constantsBuffer.Set(new ConstantsBuffer());
	this->constantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->constantsBuffer->SetShader(this->material->GetShader());
	if (!this->constantsBuffer->Setup())
	{
		THEBE_LOG("Failed to setup constants buffer.");
		return false;
	}

	DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
	if (!csuDescriptorHeap)
		return false;

	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuConstantsDescriptorSet))
	{
		THEBE_LOG("Failed to allocate constants buffer descriptor set.");
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE csuHandle;
	this->csuConstantsDescriptorSet.GetCpuHandle(0, csuHandle);
	if (!this->constantsBuffer->CreateResourceView(csuHandle, graphicsEngine->GetDevice()))
	{
		THEBE_LOG("Failed to create resource view into constants buffer.");
		return false;
	}

	return true;
}

/*virtual*/ void DynamicLineRenderer::Shutdown()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
		if (csuDescriptorHeap)
		{
			if (this->csuConstantsDescriptorSet.IsAllocated())
				csuDescriptorHeap->FreeDescriptorSet(this->csuConstantsDescriptorSet);
		}
	}

	if (this->vertexBuffer.Get())
	{
		this->vertexBuffer->Shutdown();
		this->vertexBuffer = nullptr;
	}

	this->material = nullptr;

	RenderObject::Shutdown();
}

/*virtual*/ bool DynamicLineRenderer::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	if (!this->vertexBuffer.Get() || !this->material.Get())
		return false;

	if (this->vertexBufferUpdateNeeded)
	{
		if (!this->vertexBuffer->UpdateIfNecessary(commandList))
			return false;

		this->vertexBufferUpdateNeeded = false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!context || !context->camera)
		return false;

	Matrix4x4 cameraToProjMatrix = context->camera->GetCameraToProjectionMatrix();
	Matrix4x4 worldToCameraMatrix;
	context->camera->GetWorldToCameraTransform().GetToMatrix(worldToCameraMatrix);
	Matrix4x4 worldToProjMatrix = cameraToProjMatrix * worldToCameraMatrix;
	if (!this->constantsBuffer->SetParameter("worldToProj", worldToProjMatrix))
		return false;

	if (!this->constantsBuffer->UpdateIfNecessary(commandList))
		return false;

	ID3D12PipelineState* pipelineState = graphicsEngine->GetOrCreatePipelineState(this->material, this->vertexBuffer, nullptr, context->renderTarget);
	if (!pipelineState)
		return false;

	Shader* shader = this->material->GetShader();
	commandList->SetGraphicsRootSignature(shader->GetRootSignature());
	shader->SetRootParameters(commandList, &this->csuConstantsDescriptorSet, nullptr, nullptr, nullptr);

	commandList->SetPipelineState(pipelineState);
	commandList->IASetVertexBuffers(0, 1, this->vertexBuffer->GetVertexBufferView());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->DrawInstanced(this->lineRenderCount * 2, 1, 0, 0);

	return true;
}

/*virtual*/ bool DynamicLineRenderer::RendersToTarget(RenderTarget* renderTarget) const
{
	return renderTarget->GetName() == "SwapChain";
}

bool DynamicLineRenderer::SetLine(UINT i, const Vector3& pointA, const Vector3& pointB, const Vector3* colorA /*= nullptr*/, const Vector3* colorB /*= nullptr*/)
{
	if (i >= this->lineMaxCount)
	{
		THEBE_LOG("Given index (%d) larger than max line count (%d).", i, this->lineMaxCount);
		return false;
	}

	auto vertex = &reinterpret_cast<Vertex*>(this->vertexBuffer->GetBufferPtr())[i * 2];

	vertex->x = (float)pointA.x;
	vertex->y = (float)pointA.y;
	vertex->z = (float)pointA.z;

	if (colorA)
	{
		vertex->r = (float)colorA->x;
		vertex->g = (float)colorA->y;
		vertex->b = (float)colorA->z;
	}
	else
	{
		vertex->r = 1.0f;
		vertex->g = 1.0f;
		vertex->b = 1.0f;
	}

	vertex++;

	vertex->x = (float)pointB.x;
	vertex->y = (float)pointB.y;
	vertex->z = (float)pointB.z;

	if (colorB)
	{
		vertex->r = (float)colorB->x;
		vertex->g = (float)colorB->y;
		vertex->b = (float)colorB->z;
	}
	else
	{
		vertex->r = 1.0f;
		vertex->g = 1.0f;
		vertex->b = 1.0f;
	}

	this->vertexBufferUpdateNeeded = true;
	return true;
}

bool DynamicLineRenderer::GetLine(UINT i, Vector3& pointA, Vector3& pointB, Vector3* colorA /*= nullptr*/, Vector3* colorB /*= nullptr*/) const
{
	if (i >= this->lineMaxCount)
	{
		THEBE_LOG("Given index (%d) larger than max line count (%d).", i, this->lineMaxCount);
		return false;
	}

	auto vertex = &reinterpret_cast<const Vertex*>(this->vertexBuffer->GetBufferPtr())[i * 2];

	pointA.x = vertex->x;
	pointA.y = vertex->y;
	pointA.z = vertex->z;

	if (colorA)
	{
		colorA->x = vertex->r;
		colorA->y = vertex->g;
		colorA->z = vertex->b;
	}

	vertex++;

	pointB.x = vertex->x;
	pointB.y = vertex->y;
	pointB.z = vertex->z;

	if (colorA)
	{
		colorB->x = vertex->r;
		colorB->y = vertex->g;
		colorB->z = vertex->b;
	}

	return true;
}

void DynamicLineRenderer::SetLineRenderCount(UINT lineRenderCount)
{
	this->lineRenderCount = THEBE_MIN(lineRenderCount, this->lineMaxCount);
	this->vertexBuffer->SetUploadSize(this->lineRenderCount * 2 * sizeof(Vertex));
}

UINT DynamicLineRenderer::GetLineRenderCount() const
{
	return this->lineRenderCount;
}

void DynamicLineRenderer::SetLineMaxCount(UINT lineMaxCount)
{
	this->lineMaxCount = lineMaxCount;
}

UINT DynamicLineRenderer::GetLineMaxCount() const
{
	return this->lineMaxCount;
}