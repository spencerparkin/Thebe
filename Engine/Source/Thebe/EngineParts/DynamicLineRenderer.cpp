#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//----------------------------------------- DynamicLineRenderer -----------------------------------------

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

	if (this->constantsBuffer.Get())
	{
		this->constantsBuffer->Shutdown();
		this->constantsBuffer = nullptr;
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
		this->vertexBuffer->SetUploadSize(this->lineRenderCount * 2 * sizeof(Vertex));

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

void DynamicLineRenderer::ResetLines()
{
	this->lineRenderCount = 0;
}

bool DynamicLineRenderer::AddLine(const Vector3& pointA, const Vector3& pointB, const Vector3* colorA /*= nullptr*/, const Vector3* colorB /*= nullptr*/)
{
	if (this->lineRenderCount >= this->lineMaxCount)
	{
		THEBE_LOG("Line overflow.  Max lines: %d", this->lineMaxCount);
		return false;
	}

	auto vertex = &reinterpret_cast<Vertex*>(this->vertexBuffer->GetBufferPtr())[this->lineRenderCount * 2];

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
	this->lineRenderCount++;
	return true;
}

bool DynamicLineRenderer::AddLineSet(const LineSet& lineSet, int minLine /*= -1*/, int maxLine /*= -1*/)
{
	for (int i = 0; i < (int)lineSet.pointArray.size() - 1; i += 2)
	{
		if (minLine != -1 && maxLine != -1 && !(minLine <= i / 2 && i / 2 <= maxLine))
			continue;

		const Vector3& pointA = lineSet.pointArray[i];
		const Vector3& pointB = lineSet.pointArray[i + 1];
		const Vector3& colorA = lineSet.colorArray[i];
		const Vector3& colorB = lineSet.colorArray[i + 1];

		if (!this->AddLine(pointA, pointB, &colorA, &colorB))
			return false;
	}

	return true;
}

void DynamicLineRenderer::SetLineMaxCount(UINT lineMaxCount)
{
	THEBE_ASSERT(this->vertexBuffer.Get() == nullptr);
	this->lineMaxCount = lineMaxCount;
}

UINT DynamicLineRenderer::GetLineMaxCount() const
{
	return this->lineMaxCount;
}

//----------------------------------------- DynamicLineRenderer::LineSet -----------------------------------------

DynamicLineRenderer::LineSet::LineSet()
{
}

/*virtual*/ DynamicLineRenderer::LineSet::~LineSet()
{
}

void DynamicLineRenderer::LineSet::AddLine(const Vector3& pointA, const Vector3& pointB, const Vector3* colorA /*= nullptr*/, const Vector3* colorB /*= nullptr*/)
{
	this->pointArray.push_back(pointA);
	this->pointArray.push_back(pointB);

	this->colorArray.push_back(colorA ? *colorA : Vector3(1.0, 1.0, 1.0));
	this->colorArray.push_back(colorB ? *colorB : Vector3(1.0, 1.0, 1.0));
}

void DynamicLineRenderer::LineSet::Clear()
{
	this->pointArray.clear();
	this->colorArray.clear();
}

void DynamicLineRenderer::LineSet::Dump(std::ostream& stream) const
{
	size_t size = this->pointArray.size();
	stream.write((const char*)&size, sizeof(size_t));

	for (const Vector3& point : this->pointArray)
		point.Dump(stream);

	size = this->colorArray.size();
	stream.write((const char*)&size, sizeof(size_t));

	for (const Vector3& color : this->colorArray)
		color.Dump(stream);
}

void DynamicLineRenderer::LineSet::Restore(std::istream& stream)
{
	size_t size = 0;
	stream.read((char*)&size, sizeof(size_t));

	this->pointArray.clear();
	for (size_t i = 0; i < size; i++)
	{
		Vector3 point;
		point.Restore(stream);
		this->pointArray.push_back(point);
	}

	stream.read((char*)&size, sizeof(size_t));

	this->colorArray.clear();
	for (size_t i = 0; i < size; i++)
	{
		Vector3 color;
		color.Restore(stream);
		this->colorArray.push_back(color);
	}
}