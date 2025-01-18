#include "Thebe/Network/DebugRenderClient.h"
#include "Thebe/Utilities/JsonHelper.h"

using namespace Thebe;

DebugRenderClient::DebugRenderClient()
{
}

/*virtual*/ DebugRenderClient::~DebugRenderClient()
{
}

/*virtual*/ bool DebugRenderClient::Setup()
{
	this->SetNeeds(true, false);
	
	NetworkAddress address;
	address.SetIPAddress("127.0.0.1");
	address.SetPort(2222);
	this->SetAddress(address);

	return JsonClient::Setup();
}

void DebugRenderClient::AddLine(const std::string& lineSetName, const Vector3& pointA, const Vector3& pointB, const Vector3& color)
{
	using namespace ParseParty;

	std::unique_ptr<JsonObject> rootValue(new JsonObject());

	rootValue->SetValue("line_set", new JsonString(lineSetName));
	rootValue->SetValue("action", new JsonString("add"));
	rootValue->SetValue("point_a", JsonHelper::VectorToJsonValue(pointA));
	rootValue->SetValue("point_b", JsonHelper::VectorToJsonValue(pointB));
	rootValue->SetValue("color", JsonHelper::VectorToJsonValue(color));

	this->SendJson(rootValue.get());
}

void DebugRenderClient::Clear(const std::string& lineSetName)
{
	using namespace ParseParty;

	std::unique_ptr<JsonObject> rootValue(new JsonObject());

	rootValue->SetValue("line_set", new JsonString(lineSetName));
	rootValue->SetValue("action", new JsonString("clear"));

	this->SendJson(rootValue.get());
}