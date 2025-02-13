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
	class THEBE_API Text : public Space
	{
	public:
		Text();
		virtual ~Text();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual void PrepareRenderOrder(RenderContext* context) const override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context) override;
		virtual bool RendersToTarget(RenderTarget* renderTarget) const override;
		virtual void PrepareForRender() override;
		virtual bool CanBeCollapsed() const override;

		void SetText(const std::string& text);
		const std::string& GetText() const;

		void SetFont(Font* font);
		Font* GetFont();
		const Font* GetFont() const;
		void SetMaxCharacters(UINT maxCharacters);
		UINT GetMaxCharacters() const;
		void SetFontSize(double fontSize);
		double GetFontSize() const;
		void SetTextColor(const Vector3& textColor);
		const Vector3& GetTextColor() const;

		enum RenderSpace
		{
			WORLD,
			CAMERA,
			BILLBOARD
		};

		void SetRenderSpace(RenderSpace renderSpace);
		RenderSpace GetRenderSpace() const;

	protected:
		struct CharRenderInfo
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
		RenderSpace renderSpace;
	};

	/**
	 * 
	 */
	class THEBE_API FramerateText : public Text
	{
	public:
		FramerateText();
		virtual ~FramerateText();

		virtual bool Setup() override;
		virtual void PrepareForRender() override;

		void ResetWaterMarks();

	private:
		std::list<double> deltaTimeList;
		size_t deltaTimeListSizeMax;
		double frameRateLowWaterMark;
		double frameRateHighWaterMark;
		UINT64 waterMarkResetFrequency;
	};

	/**
	 * 
	 */
	class THEBE_API ProfileTreeText : public Text
	{
	public:
		ProfileTreeText();
		virtual ~ProfileTreeText();

		virtual bool Setup() override;
		virtual void PrepareForRender() override;
	};
}