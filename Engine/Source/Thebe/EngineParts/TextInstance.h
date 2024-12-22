#pragma once

#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/EngineParts/VertexBuffer.h"

namespace Thebe
{
	class Font;
	class StructuredBuffer;
	class VertexBuffer;
	class ConstantsBuffer;

	/**
	 * 
	 */
	class THEBE_API TextInstance : public Space
	{
	public:
		TextInstance();
		virtual ~TextInstance();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual uint32_t GetRenderOrder() const override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context) override;
		virtual bool RendersToTarget(RenderTarget* renderTarget) const override;
		virtual void PrepareForRender() override;

		void SetText(const std::string& text);
		const std::string& GetText() const;

		void SetFont(Font* font);
		Font* GetFont();
		const Font* GetFont() const;
		void SetMaxCharacters(UINT maxCharacter);
		UINT GetMaxCharacters() const;
		void SetFontSize(double fontSize);
		double GetFontSize() const;
		void SetTextColor(const Vector3& textColor);
		const Vector3& GetTextColor() const;

	private:
		struct CharInfo
		{
			float minU;
			float minV;
			float maxU;
			float maxV;
			float scaleX;
			float scaleY;
			float deltaX;
			float deltaY;
		};

		UINT maxCharacters;
		std::string text;
		std::string renderedText;
		Reference<Font> font;
		Reference<StructuredBuffer> charBuffer;
		Reference<VertexBuffer> vertexBuffer;
		Reference<ConstantsBuffer> constantsBuffer;
		DescriptorHeap::DescriptorSet csuCharBufferDescriptorSet;
		DescriptorHeap::DescriptorSet csuConstantsBufferDescriptorSet;
		DescriptorHeap::DescriptorSet csuAtlasTextureDescriptorSet;
		bool charBufferUpdateNeeded;
		double fontSize;
		Vector3 textColor;
		UINT numCharsToRender;
	};
}