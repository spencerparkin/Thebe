#include "Thebe/Network/DebugRenderServer.h"
#include "Thebe/Utilities/JsonHelper.h"

using namespace Thebe;

//-------------------------------------- DebugRenderServer --------------------------------------

DebugRenderServer::DebugRenderServer()
{
}

/*virtual*/ DebugRenderServer::~DebugRenderServer()
{
}

void DebugRenderServer::Draw(Thebe::DynamicLineRenderer* lineRenderer) const
{
	for (const auto& pair : this->lineSetMap)
	{
		const LineSet* lineSet = pair.second;
		lineSet->Draw(lineRenderer);
	}
}

/*virtual*/ bool DebugRenderServer::Setup()
{
	this->SetNeeds(false, true);
	this->SetMaxConnections(1);

	NetworkAddress address;
	address.SetIPAddress("127.0.0.1");
	address.SetPort(2222);
	this->SetAddress(address);

	return JsonServer::Setup();
}

/*virtual*/ void DebugRenderServer::ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply)
{
	using namespace ParseParty;

	auto rootValue = dynamic_cast<const JsonObject*>(message->jsonValue);
	if (!rootValue)
		return;

	auto lineSetValue = dynamic_cast<const JsonString*>(rootValue->GetValue("line_set"));
	if (!lineSetValue)
		return;

	std::string lineSetName = lineSetValue->GetValue();
	Reference<LineSet> lineSet;
	auto pair = this->lineSetMap.find(lineSetName);
	if (pair == this->lineSetMap.end())
		lineSet.Set(new LineSet());
	else
		lineSet = pair->second;

	auto actionValue = dynamic_cast<const JsonString*>(rootValue->GetValue("action"));
	if (!actionValue)
		return;

	std::string action = actionValue->GetValue();
	if (action == "clear")
		lineSet->Clear();
	else if (action == "add")
	{
		Line line;

		if (!JsonHelper::VectorFromJsonValue(rootValue->GetValue("color"), line.color))
			return;

		if (!JsonHelper::VectorFromJsonValue(rootValue->GetValue("point_a"), line.line.point[0]))
			return;

		if (!JsonHelper::VectorFromJsonValue(rootValue->GetValue("point_b"), line.line.point[1]))
			return;

		lineSet->Add(line);
	}
}

//-------------------------------------- DebugRenderServer::LineSet --------------------------------------

DebugRenderServer::LineSet::LineSet()
{
}

/*virtual*/ DebugRenderServer::LineSet::~LineSet()
{
}

void DebugRenderServer::LineSet::Draw(Thebe::DynamicLineRenderer* lineRenderer) const
{
	for (const Line& line : this->lineArray)
		lineRenderer->AddLine(line.line.point[0], line.line.point[1], &line.color, &line.color);
}

void DebugRenderServer::LineSet::Add(const Line& line)
{
	this->lineArray.push_back(line);
}

void DebugRenderServer::LineSet::Clear()
{
	this->lineArray.clear();
}