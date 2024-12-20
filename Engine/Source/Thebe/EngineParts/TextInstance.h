#pragma once

#include "Thebe/EngineParts/Space.h"

namespace Thebe
{
	class Font;

	/**
	 * You can think of these as instances of a font being rendered in the scene.
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

		void SetText(const std::string& text);
		const std::string& GetText() const;

		void SetFont(Font* font);
		Font* GetFont();
		const Font* GetFont() const;

	private:
		std::string text;
		Reference<Font> font;
	};
}