#include "Thebe/EngineParts/TextInstance.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/RenderTarget.h"

using namespace Thebe;

TextInstance::TextInstance()
{
}

/*virtual*/ TextInstance::~TextInstance()
{
}

/*virtual*/ bool TextInstance::Setup()
{
	if (!Space::Setup())
		return false;

	//...create dynamic vertex buffer here...

	return true;
}

/*virtual*/ void TextInstance::Shutdown()
{
	Space::Shutdown();
}

/*virtual*/ bool TextInstance::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
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