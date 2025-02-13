#include "Application.h"
#include "LifeText.h"
#include "HumanClient.h"
#include "Factory.h"
#include "Thebe/EngineParts/Font.h"
#include <format>

using namespace Thebe;

LifeText::LifeText()
{
}

/*virtual*/ LifeText::~LifeText()
{
}

/*virtual*/ bool LifeText::Setup()
{
	if (!this->font)
	{
		Reference<GraphicsEngine> graphicsEngine;
		if (!this->GetGraphicsEngine(graphicsEngine))
			return false;

		if (!graphicsEngine->LoadEnginePartFromFile(R"(Fonts\Roboto_Regular.font)", this->font))
			return false;
	}

	this->SetRenderSpace(RenderSpace::BILLBOARD);

	if (!Text::Setup())
		return false;

	return true;
}

/*virtual*/ void LifeText::PrepareForRender()
{
	std::string numLivesText;

	std::shared_ptr<ChineseCheckers::Marble> nativeMarble(this->marbleWeakRef);
	if (nativeMarble.get())
	{
		auto marble = dynamic_cast<Marble*>(nativeMarble.get());
		THEBE_ASSERT_FATAL(marble != nullptr);
		numLivesText = std::format("{}", marble->numLives);
	}

	this->SetText(numLivesText);

	Text::PrepareForRender();
}