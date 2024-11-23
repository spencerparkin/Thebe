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

bool EnginePart::GetGraphicsEngine(Reference<GraphicsEngine>& graphicsEngine)
{
	Reference<ReferenceCounted> ref;
	if (!HandleManager::Get()->GetObjectFromHandle(this->graphicsEngineHandle, ref))
		return false;

	graphicsEngine.SafeSet(ref.Get());
	return graphicsEngine.Get() ? true : false;
}

/*virtual*/ bool EnginePart::LoadFromJson(const ParseParty::JsonValue* jsonValue)
{
	THEBE_LOG("Loading engine part from JSON is not supported for this type of part.");
	return false;
}

/*virtual*/ bool EnginePart::DumpToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const
{
	THEBE_LOG("Dumping engine part to JSON is not supported for this type of part.");
	return false;
}