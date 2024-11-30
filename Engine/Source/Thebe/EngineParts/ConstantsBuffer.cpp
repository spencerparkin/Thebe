#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

ConstantsBuffer::ConstantsBuffer()
{
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

/*virtual*/ ConstantsBuffer::~ConstantsBuffer()
{
	this->sizeAlignmentRequirement = 256;
}

void ConstantsBuffer::SetShader(Shader* shader)
{
	this->shader = shader;
}

/*virtual*/ bool ConstantsBuffer::Setup()
{
	if (!this->shader)
	{
		THEBE_LOG("No shader configured for constants buffer.");
		return false;
	}

	this->SetBufferType(Buffer::DYNAMIC);

	// Note that some error checking could be done here to make sure
	// that each parameter satisfies the alignment and offset rules
	// that are required.  My script does that for now.
	const std::vector<Shader::Parameter>& parameterArray = this->shader->GetParameterArray();
	UINT32 constantsBufferSize = 0;
	for (const auto& parameter : parameterArray)
	{
		UINT32 size = parameter.offset + parameter.GetSize();
		if (constantsBufferSize < size)
			constantsBufferSize = size;
	}

	constantsBufferSize = THEBE_ALIGNED(constantsBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	if (constantsBufferSize == 0)
	{
		THEBE_LOG("Failed to setup constants buffer.  Constants buffer size is zero.");
		return false;
	}

	this->originalBuffer.resize(constantsBufferSize);
	::ZeroMemory(this->originalBuffer.data(), constantsBufferSize);

	if (!Buffer::Setup())
	{
		THEBE_LOG("Failed to setup constants buffer.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	DescriptorHeap* cbvDescriptorHeap = graphicsEngine->GetCbvDescriptorHeap();
	if (!cbvDescriptorHeap->AllocDescriptor(this->cbvDescriptor))
	{
		THEBE_LOG("Failed to allocated descriptor from CBV descriptor heap.");
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = this->gpuBuffer->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = this->gpuBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = resourceDesc.Width;
	graphicsEngine->GetDevice()->CreateConstantBufferView(&cbvDesc, this->cbvDescriptor.cpuHandle);

	return true;
}

/*virtual*/ void ConstantsBuffer::Shutdown()
{
	if (this->cbvDescriptor.heapHandle != THEBE_INVALID_REF_HANDLE)
	{
		Reference<GraphicsEngine> graphicsEngine;
		if (this->GetGraphicsEngine(graphicsEngine))
		{
			DescriptorHeap* cbvDescriptorHeap = graphicsEngine->GetCbvDescriptorHeap();
			cbvDescriptorHeap->FreeDescriptor(this->cbvDescriptor);
		}
	}

	Buffer::Shutdown();
}

const DescriptorHeap::Descriptor& ConstantsBuffer::GetDescriptor() const
{
	return this->cbvDescriptor;
}

bool ConstantsBuffer::HasParameter(const std::string& name)
{
	return this->shader->FindParameter(name) ? true : false;
}

Shader::Parameter::Type ConstantsBuffer::GetParameterType(const std::string& name)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return Shader::Parameter::Type::UNKNOWN;

	return parameter->type;
}

bool ConstantsBuffer::SetParameter(const std::string& name, double scalar)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return false;

	if (parameter->type != Shader::Parameter::FLOAT)
		return false;

	float floatArray[] = { float(scalar) };
	::memcpy(&this->GetBufferPtr()[parameter->offset], floatArray, sizeof(floatArray));
	return true;
}

bool ConstantsBuffer::SetParameter(const std::string& name, const Vector2& vector)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return false;

	if (parameter->type != Shader::Parameter::FLOAT2)
		return false;

	float floatArray[] = { float(vector.x), float(vector.y) };
	::memcpy(&this->GetBufferPtr()[parameter->offset], floatArray, sizeof(floatArray));
	return true;
}

bool ConstantsBuffer::SetParameter(const std::string& name, const Vector3& vector)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return false;

	if (parameter->type != Shader::Parameter::FLOAT3)
		return false;

	float floatArray[] = { float(vector.x), float(vector.y), float(vector.z) };
	::memcpy(&this->GetBufferPtr()[parameter->offset], floatArray, sizeof(floatArray));
	return true;
}

bool ConstantsBuffer::SetParameter(const std::string& name, const Matrix2x2& matrix)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return false;

	if (parameter->type != Shader::Parameter::FLOAT2x2)
		return false;

	float floatArray[] =
	{
		(float)matrix.ele[0][0], (float)matrix.ele[1][0],
		(float)matrix.ele[0][1], (float)matrix.ele[1][1]
	};
	::memcpy(&this->GetBufferPtr()[parameter->offset], floatArray, sizeof(floatArray));
	return true;
}

bool ConstantsBuffer::SetParameter(const std::string& name, const Matrix3x3& matrix)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return false;

	if (parameter->type != Shader::Parameter::FLOAT3x3)
		return false;

	float floatArray[] =
	{
		(float)matrix.ele[0][0], (float)matrix.ele[1][0], (float)matrix.ele[2][0],
		(float)matrix.ele[0][1], (float)matrix.ele[1][1], (float)matrix.ele[2][1],
		(float)matrix.ele[0][2], (float)matrix.ele[1][2], (float)matrix.ele[2][2]
	};
	::memcpy(&this->GetBufferPtr()[parameter->offset], floatArray, sizeof(floatArray));
	return true;
}

bool ConstantsBuffer::SetParameter(const std::string& name, const Matrix4x4& matrix)
{
	const Shader::Parameter* parameter = this->shader->FindParameter(name);
	if (!parameter)
		return false;

	if (parameter->type != Shader::Parameter::FLOAT4x4)
		return false;

	float floatArray[] =
	{
		(float)matrix.ele[0][0], (float)matrix.ele[1][0], (float)matrix.ele[2][0], (float)matrix.ele[3][0],
		(float)matrix.ele[0][1], (float)matrix.ele[1][1], (float)matrix.ele[2][1], (float)matrix.ele[3][1],
		(float)matrix.ele[0][2], (float)matrix.ele[1][2], (float)matrix.ele[2][2], (float)matrix.ele[3][2],
		(float)matrix.ele[0][3], (float)matrix.ele[1][3], (float)matrix.ele[2][3], (float)matrix.ele[3][3]
	};
	::memcpy(&this->GetBufferPtr()[parameter->offset], floatArray, sizeof(floatArray));
	return true;
}