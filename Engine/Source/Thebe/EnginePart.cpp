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

/*virtual*/ bool EnginePart::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{
	return false;
}

/*virtual*/ bool EnginePart::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const
{
	return false;
}