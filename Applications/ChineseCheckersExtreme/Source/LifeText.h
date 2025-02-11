#pragma once

#include "Thebe/EngineParts/Text.h"
#include "ChineseCheckers/Marble.h"

class LifeText : public Thebe::Text
{
public:
	LifeText();
	virtual ~LifeText();

	virtual bool Setup() override;
	virtual void PrepareForRender() override;

	std::weak_ptr<ChineseCheckers::Marble> marbleWeakRef;
};