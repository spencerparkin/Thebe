#include "Thebe/EngineParts/Text.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/StructuredBuffer.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Profiler.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------ Text ------------------------------

Text::Text()
{
	this->maxCharacters = 256;
	this->fontSize = 1.0;
	this->charBufferUpdateNeeded = false;
	this->numCharsToRender = 0;
	this->renderSpace = RenderSpace::CAMERA;
	this->textColor.SetComponents(1.0, 0.0, 0.0);
}

/*virtual*/ Text::~Text()
{
}

/*virtual*/ bool Text::CanBeCollapsed() const
{
	return false;
}

/*virtual*/ bool Text::Setup()
{
	if (!this->font.Get())
	{
		THEBE_LOG("No font configured.");
		return false;
	}

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
	this->charBuffer->SetStructSize(sizeof(CharRenderInfo));

	std::vector<UINT8>& originalBuffer = this->charBuffer->GetOriginalBuffer();
	originalBuffer.resize(this->maxCharacters * sizeof(CharRenderInfo));
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

	Shader* fontShader = this->font->GetShader();
	this->constantsBuffer.Set(new ConstantsBuffer());
	this->constantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->constantsBuffer->SetShader(fontShader);
	this->constantsBuffer->SetName("ConstantsBufferForTextInstance");
	if (!this->constantsBuffer->Setup())
	{
		THEBE_LOG("Failed to setup constants buffer for text instance.");
		return false;
	}

	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuConstantsBufferDescriptorSet))
	{
		THEBE_LOG("Failed to allocate constants buffer descriptor set.");
		return false;
	}

	this->csuConstantsBufferDescriptorSet.GetCpuHandle(0, csuHandle);
	this->constantsBuffer->CreateResourceView(csuHandle, graphicsEngine->GetDevice());

	if (fontShader->GetTextureUsageForRegister(0) != "char_atlas")
	{
		THEBE_LOG("Expected texture register 0 to be used for \"char_atlas\".");
		return false;
	}

	Buffer* buffer = this->font->GetTextureForRegister(0);
	if (!buffer)
	{
		THEBE_LOG("Failed to get texture for register 0.");
		return false;
	}

	if (!csuDescriptorHeap->AllocDescriptorSet(1, this->csuAtlasTextureDescriptorSet))
	{
		THEBE_LOG("Failed to allocate atlas texture descriptor set.");
		return false;
	}

	this->csuAtlasTextureDescriptorSet.GetCpuHandle(0, csuHandle);
	buffer->CreateResourceView(csuHandle, graphicsEngine->GetDevice());

	return true;
}

/*virtual*/ void Text::Shutdown()
{
	if (this->charBuffer.Get())
	{
		this->charBuffer->Shutdown();
		this->charBuffer = nullptr;
	}

	if (this->constantsBuffer.Get())
	{
		this->constantsBuffer->Shutdown();
		this->constantsBuffer = nullptr;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap();

		if (this->csuCharBufferDescriptorSet.IsAllocated())
			csuDescriptorHeap->FreeDescriptorSet(this->csuCharBufferDescriptorSet);

		if (this->csuConstantsBufferDescriptorSet.IsAllocated())
			csuDescriptorHeap->FreeDescriptorSet(this->csuConstantsBufferDescriptorSet);

		if (this->csuAtlasTextureDescriptorSet.IsAllocated())
			csuDescriptorHeap->FreeDescriptorSet(this->csuAtlasTextureDescriptorSet);
	}

	// Don't shutdown this buffer.  Some other instance might be using it.
	this->vertexBuffer = nullptr;

	Space::Shutdown();
}

/*virtual*/ void Text::PrepareForRender()
{
	Space::PrepareForRender();

	if (this->renderedText == this->text)
		return;

	if (this->text.length() > this->maxCharacters)
	{
		THEBE_LOG("String of length (%d) exceeds maximum character length (%d).", this->text.length(), this->maxCharacters);
		return;
	}

	const std::vector<Font::CharacterInfo>& charInfoArray = this->font->GetCharacterInfoArray();
	auto charRenderInfo = reinterpret_cast<CharRenderInfo*>(this->charBuffer->GetBufferPtr());

	Vector2 penPosition(0.0, 0.0);
	this->numCharsToRender = 0;

	for (UINT i = 0; i < this->maxCharacters; i++)
	{
		if (this->text.c_str()[i] == '\0')
			break;

		if (this->text.c_str()[i] == '\n')
		{
			penPosition.x = 0.0;
			penPosition.y -= this->font->GetLineHeight() * this->fontSize;
			continue;
		}

		const Font::CharacterInfo& charInfo = charInfoArray[this->text.c_str()[i]];

		charRenderInfo->minU = (float)charInfo.minUV.x;
		charRenderInfo->minV = (float)charInfo.minUV.y;
		charRenderInfo->maxU = (float)charInfo.maxUV.x;
		charRenderInfo->maxV = (float)charInfo.maxUV.y;

		charRenderInfo->scaleX = float((charInfo.maxUV.x - charInfo.minUV.x) * this->fontSize);
		charRenderInfo->scaleY = float((charInfo.maxUV.y - charInfo.minUV.y) * this->fontSize);

		Vector2 charLocation = penPosition + charInfo.penOffset * this->fontSize;

		charRenderInfo->deltaX = (float)charLocation.x;
		charRenderInfo->deltaY = (float)charLocation.y;

		penPosition.x += charInfo.advance * this->fontSize;
		charRenderInfo++;
		this->numCharsToRender++;
	}

	this->charBufferUpdateNeeded = true;
}

/*virtual*/ bool Text::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	if (!this->charBuffer.Get())
		return false;

	if (this->text.length() > this->maxCharacters)
	{
		THEBE_LOG("Text length exceeds max characters.");
		return false;
	}

	if (this->charBufferUpdateNeeded)
	{
		if (!this->charBuffer->UpdateIfNecessary(commandList))
			return false;

		this->charBufferUpdateNeeded = false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!context || !context->camera)
		return false;

	switch (this->renderSpace)
	{
		case RenderSpace::WORLD:
		{
			Matrix4x4 objectToProjMatrix, objectToCameraMatrix, objectToWorldMatrix;
			this->CalcGraphicsMatrices(context->camera, objectToProjMatrix, objectToCameraMatrix, objectToWorldMatrix);
			this->constantsBuffer->SetParameter("objToProj", objectToProjMatrix);
			break;
		}
		case RenderSpace::CAMERA:
		{
			Matrix4x4 cameraToProjMatrix = context->camera->GetCameraToProjectionMatrix();
			Matrix4x4 objectToCameraMatrix;
			this->objectToWorld.GetToMatrix(objectToCameraMatrix);
			Matrix4x4 objectToProjMatrix = cameraToProjMatrix * objectToCameraMatrix;
			this->constantsBuffer->SetParameter("objToProj", objectToProjMatrix);
			break;
		}
		default:
		{
			THEBE_LOG("Render space (%d) unknown.", this->renderSpace);
			return false;
		}
	}

	this->constantsBuffer->SetParameter("textColor", this->textColor);

	if (!this->constantsBuffer->UpdateIfNecessary(commandList))
	{
		THEBE_LOG("Failed to update constants buffer for text instance.");
		return false;
	}

	ID3D12PipelineState* pipelineState = graphicsEngine->GetOrCreatePipelineState(this->font, this->vertexBuffer, nullptr, context->renderTarget);
	if (!pipelineState)
		return false;

	Shader* fontShader = this->font->GetShader();

	commandList->SetGraphicsRootSignature(fontShader->GetRootSignature());
	
	fontShader->SetRootParameters(commandList,
		&this->csuConstantsBufferDescriptorSet,
		&this->csuAtlasTextureDescriptorSet,
		nullptr,
		&this->csuCharBufferDescriptorSet);

	commandList->SetPipelineState(pipelineState);
	commandList->IASetVertexBuffers(0, 1, this->vertexBuffer->GetVertexBufferView());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, this->numCharsToRender, 0, 0);

	return true;
}

/*virtual*/ bool Text::RendersToTarget(RenderTarget* renderTarget) const
{
	if (renderTarget->GetName() == "ShadowBuffer")
		return false;

	return true;
}

/*virtual*/ uint32_t Text::GetRenderOrder() const
{
	return THEBE_RENDER_ORDER_TEXT;
}

void Text::SetText(const std::string& text)
{
	this->text = text;
}

const std::string& Text::GetText() const
{
	return this->text;
}

void Text::SetFont(Font* font)
{
	this->font = font;
}

Font* Text::GetFont()
{
	return this->font;
}

const Font* Text::GetFont() const
{
	return this->font;
}

void Text::SetMaxCharacters(UINT maxCharacter)
{
	this->maxCharacters = maxCharacters;
}

UINT Text::GetMaxCharacters() const
{
	return this->maxCharacters;
}

void Text::SetFontSize(double fontSize)
{
	this->fontSize = fontSize;
}

double Text::GetFontSize() const
{
	return this->fontSize;
}

void Text::SetTextColor(const Vector3& textColor)
{
	this->textColor = textColor;
}

const Vector3& Text::GetTextColor() const
{
	return this->textColor;
}

void Text::SetRenderSpace(RenderSpace renderSpace)
{
	this->renderSpace = renderSpace;
}

Text::RenderSpace Text::GetRenderSpace() const
{
	return this->renderSpace;
}

//------------------------------ FramerateText ------------------------------

FramerateText::FramerateText()
{
	this->deltaTimeListSizeMax = 16;
	this->waterMarkResetFrequency = 128;
	this->ResetWaterMarks();
}

/*virtual*/ FramerateText::~FramerateText()
{
}

/*virtual*/ bool FramerateText::Setup()
{
	if (!this->font)
	{
		Reference<GraphicsEngine> graphicsEngine;
		if (!this->GetGraphicsEngine(graphicsEngine))
			return false;

		if (!graphicsEngine->LoadEnginePartFromFile(R"(Fonts\Roboto_Regular.font)", this->font))
			return false;
	}

	this->SetRenderSpace(CAMERA);

	Transform objectToScreen;
	objectToScreen.SetIdentity();
	objectToScreen.translation.SetComponents(-1.0, 0.5, -2.0);
	this->SetChildToParentTransform(objectToScreen);

	if (!Text::Setup())
		return false;

	return true;
}

/*virtual*/ void FramerateText::PrepareForRender()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		double deltaTimeSeconds = graphicsEngine->GetDeltaTime();
		this->deltaTimeList.push_back(deltaTimeSeconds);
		while (this->deltaTimeList.size() > this->deltaTimeListSizeMax)
			this->deltaTimeList.pop_front();
	}

	if (graphicsEngine->GetFrameCount() % this->waterMarkResetFrequency == 0)
		this->ResetWaterMarks();

	if (this->deltaTimeList.size() == 0)
		this->SetText("Framerate: ?");
	else
	{
		double averageDeltaTime = 0.0;
		for (double deltaTime : this->deltaTimeList)
			averageDeltaTime += deltaTime;
		averageDeltaTime /= double(this->deltaTimeList.size());
		double framerateFPS = 1.0 / averageDeltaTime;

		if (framerateFPS > this->frameRateHighWaterMark)
			this->frameRateHighWaterMark = framerateFPS;

		if (framerateFPS < this->frameRateLowWaterMark)
			this->frameRateLowWaterMark = framerateFPS;

		this->SetText(std::format("Framerate: {:2.2f}\nBest FPS: {:2.2f}\nWorst FPS: {:2.2f}",
									framerateFPS,
									this->frameRateHighWaterMark,
									this->frameRateLowWaterMark));
	}

	Text::PrepareForRender();
}

void FramerateText::ResetWaterMarks()
{
	this->frameRateLowWaterMark = std::numeric_limits<double>::max();
	this->frameRateHighWaterMark = -std::numeric_limits<double>::max();
}

//------------------------------ ProfileTreeText ------------------------------

ProfileTreeText::ProfileTreeText()
{
}

/*virtual*/ ProfileTreeText::~ProfileTreeText()
{
}

/*virtual*/ bool ProfileTreeText::Setup()
{
	if (!this->font)
	{
		Reference<GraphicsEngine> graphicsEngine;
		if (!this->GetGraphicsEngine(graphicsEngine))
			return false;

		if (!graphicsEngine->LoadEnginePartFromFile(R"(Fonts\Roboto_Regular.font)", this->font))
			return false;
	}

	this->SetRenderSpace(CAMERA);

	Transform objectToScreen;
	objectToScreen.SetIdentity();
	objectToScreen.translation.SetComponents(-1.0, 0.5, -2.0);
	this->SetChildToParentTransform(objectToScreen);

	if (!Text::Setup())
		return false;

	return true;
}

/*virtual*/ void ProfileTreeText::PrepareForRender()
{
	std::string profileText;

	const Profiler::PersistentRecord* record = Profiler::Get()->GetProfileTree();
	if (record)
		profileText = record->GenerateText();

	this->SetText(profileText);

	Text::PrepareForRender();
}