#include "Marble.h"
#include "Graph.h"

using namespace ChineseCheckers;

Marble::Marble(Color color)
{
	this->color = color;
}

/*virtual*/ Marble::~Marble()
{
}

Marble::Color Marble::GetColor() const
{
	return this->color;
}

/*virtual*/ bool Marble::ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const
{
	using namespace ParseParty;

	auto marbleValue = new JsonObject();
	jsonValue.reset(marbleValue);

	marbleValue->SetValue("color", new JsonInt(this->color));

	return true;
}

/*virtual*/ bool Marble::FromJson(const ParseParty::JsonValue* jsonValue)
{
	using namespace ParseParty;

	auto marbleValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!marbleValue)
		return false;

	auto colorValue = dynamic_cast<const JsonInt*>(marbleValue->GetValue("color"));
	if (!colorValue)
		return false;

	this->color = (Color)colorValue->GetValue();

	return true;
}