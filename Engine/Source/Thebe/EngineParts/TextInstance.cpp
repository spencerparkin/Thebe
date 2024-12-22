#include "Thebe/EngineParts/TextInstance.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/StructuredBuffer.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

TextInstance::TextInstance()
{
	this->maxCharacters = 256;
}

/*virtual*/ TextInstance::~TextInstance()
{
}

/*virtual*/ bool TextInstance::Setup()
{
	if (!Space::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!graphicsEngine->LoadEnginePartFromFile("Buffers/CharacterQuad.vertex_buffer", this->vertexBuffer))
	{
		THEBE_LOG("Failed to load character quad vertex buffer.");
		return false;
	}

	if (this->maxCharacters == 0)
	{
		THEBE_LOG("Max characters is zero.");
		return false;
	}

	this->charBuffer.Set(new StructuredBuffer());
	this->charBuffer->SetGraphicsEngine(graphicsEngine);
	this->charBuffer->SetBufferType(Buffer::DYNAMIC);
	this->charBuffer->SetStructSize(sizeof(CharInfo));

	std::vector<UINT8>& originalBuffer = this->charBuffer->GetOriginalBuffer();
	originalBuffer.resize(this->maxCharacters * sizeof(CharInfo));
	::memset(originalBuffer.data(), 0, originalBuffer.size());

	if (!this->charBuffer->Setup())
	{
		THEBE_LOG("Failed to setup character buffer.");
		return false;
	}

	DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuCharBufferDescriptorSet))
	{
		THEBE_LOG("Failed to allocate character buffer descriptor set.");
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE csuHandle;
	this->csuCharBufferDescriptorSet.GetCpuHandle(0, csuHandle);
	this->charBuffer->CreateResourceView(csuHandle, graphicsEngine->GetDevice());

	return true;
}

/*virtual*/ void TextInstance::Shutdown()
{
	if (this->charBuffer.Get())
	{
		this->charBuffer->Shutdown();
		this->charBuffer = nullptr;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		if (this->csuCharBufferDescriptorSet.IsAllocated())
		{
			DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();
			csuDescriptorHeap->FreeDescriptorSet(this->csuCharBufferDescriptorSet);
		}
	}

	// Don't shutdown this buffer.  Some other instance might be using it.
	this->vertexBuffer = nullptr;

	Space::Shutdown();
}

/*virtual*/ void TextInstance::PrepareForRender()
{
	if (this->renderedText == this->text)
		return;

	//...
}

/*virtual*/ bool TextInstance::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	if (!this->charBuffer.Get())
		return false;

	if (this->renderedText != this->text)
	{
		if (!this->charBuffer->UpdateIfNecessary(commandList))
			return false;

		this->renderedText = this->text;
	}

	//...

	return true;
}

/*virtual*/ bool TextInstance::RendersToTarget(RenderTarget* renderTarget) const
{
	if (renderTarget->GetName() == "ShadowBuffer")
		return false;

	return true;
}

/*virtual*/ uint32_t TextInstance::GetRenderOrder() const
{
	return THEBE_RENDER_ORDER_ALPHA_BLEND;
}

void TextInstance::SetText(const std::string& text)
{
	this->text = text;
}

const std::string& TextInstance::GetText() const
{
	return this->text;
}

void TextInstance::SetFont(Font* font)
{
	this->font = font;
}

Font* TextInstance::GetFont()
{
	return this->font;
}

const Font* TextInstance::GetFont() const
{
	return this->font;
}

void TextInstance::SetMaxCharacters(UINT maxCharacter)
{
	this->maxCharacters = maxCharacters;
}

UINT TextInstance::GetMaxCharacters() const
{
	return this->maxCharacters;
}