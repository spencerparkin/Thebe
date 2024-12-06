#include "Thebe/EnginePart.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

EnginePart::EnginePart()
{
	this->graphicsEngineHandle = THEBE_INVALID_REF_HANDLE;
}

/*virtual*/ EnginePart::~EnginePart()
{
}

void EnginePart::SetName(const std::string& name)
{
	this->name = name;
}

const std::string& EnginePart::GetName() const
{
	return this->name;
}

void EnginePart::SetGraphicsEngine(GraphicsEngine* graphicsEngine)
{
	this->graphicsEngineHandle = graphicsEngine->GetHandle();
}

bool EnginePart::GetGraphicsEngine(Reference<GraphicsEngine>& graphicsEngine) const
{
	Reference<ReferenceCounted> ref;
	if (!HandleManager::Get()->GetObjectFromHandle(this->graphicsEngineHandle, ref))
		return false;

	graphicsEngine.SafeSet(ref.Get());
	return graphicsEngine.Get() ? true : false;
}

/*virtual*/ bool EnginePart::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root JSON value to be an object.");
		return false;
	}

	auto nameValue = dynamic_cast<const JsonString*>(rootValue->GetValue("name"));
	if (nameValue)
		this->name = nameValue->GetValue();
	else
		this->name = "";

	return false;
}

/*virtual*/ bool EnginePart::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	auto rootValue = new JsonObject();
	jsonValue.reset(rootValue);

	if (this->name.length() > 0)
		rootValue->SetValue("name", new JsonString(this->name.c_str()));

	return true;
}