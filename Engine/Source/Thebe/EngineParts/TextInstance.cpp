#include "Thebe/EngineParts/TextInstance.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/StructuredBuffer.h"
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

	if (this->maxCharacters == 0)
	{
		THEBE_LOG("Max characters is zero.");
		return false;
	}

	this->charBuffer.Set(new StructuredBuffer());
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

	//csuCharBufferDescriptorSet

	return true;
}

/*virtual*/ void TextInstance::Shutdown()
{
	if (this->charBuffer.Get())
	{
		this->charBuffer->Shutdown();
		this->charBuffer = nullptr;
	}

	Space::Shutdown();
}

/*virtual*/ void TextInstance::PrepareForRender()
{
	if (this->renderedText == this->text)
		return;
}

/*virtual*/ bool TextInstance::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	if (this->renderedText != this->text)
	{
		//if (!this->vertexBuffer->UpdateIfNecessary(commandList))
		//	return false;

		this->renderedText = this->text;
	}

	//...

	// TODO: Remember to bind both the vertex buffer and the structured buffer with IASetVertexBuffers().

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