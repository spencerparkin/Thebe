#include "Canvas.h"
#include "Application.h"
#include "HumanClient.h"
#include "Factory.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/CollisionSystem.h"
#include "Thebe/PhysicsSystem.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/Profiler.h"

using namespace Thebe;

ChineseCheckersCanvas::ChineseCheckersCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &ChineseCheckersCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &ChineseCheckersCanvas::OnSize, this);
	this->Bind(wxEVT_MOTION, &ChineseCheckersCanvas::OnMouseMotion, this);
	this->Bind(wxEVT_LEFT_DOWN, &ChineseCheckersCanvas::OnMouseLeftClick, this);
	this->Bind(wxEVT_RIGHT_DOWN, &ChineseCheckersCanvas::OnMouseRightClick, this);
	this->Bind(wxEVT_MIDDLE_DOWN, &ChineseCheckersCanvas::OnMouseMiddleClick, this);
	this->Bind(wxEVT_KEY_UP, &ChineseCheckersCanvas::OnKeyUp, this);

	this->debugDraw = false;
}

/*virtual*/ ChineseCheckersCanvas::~ChineseCheckersCanvas()
{
}

void ChineseCheckersCanvas::OnKeyUp(wxKeyEvent& event)
{
	if (event.GetKeyCode() == 'P')
	{
		GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
		auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
		if (scene)
		{
			Space* profileText = scene->GetRootSpace()->FindSpaceByName("ProfileText");
			if (profileText)
			{
				uint32_t flags = profileText->GetFlags();
				flags ^= THEBE_RENDER_OBJECT_FLAG_VISIBLE;
				profileText->SetFlags(flags);
			}
		}
	}
}

void ChineseCheckersCanvas::OnPaint(wxPaintEvent& event)
{
	THEBE_PROFILE_BEGIN_FRAME;

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CollisionSystem* collisionSystem = graphicsEngine->GetCollisionSystem();
	PhysicsSystem* physicsSystem = graphicsEngine->GetPhysicsSystem();
	EventSystem* eventSystem = graphicsEngine->GetEventSystem();

	DynamicLineRenderer* lineRenderer = wxGetApp().GetLineRenderer();
	if (lineRenderer)
	{
		lineRenderer->ResetLines();

		if (this->debugDraw)
		{
			collisionSystem->DebugDraw(lineRenderer);
			physicsSystem->DebugDraw(lineRenderer);
		}
	}

	double deltaTimeSeconds = graphicsEngine->GetDeltaTime();
	physicsSystem->StepSimulation(deltaTimeSeconds, collisionSystem);

	eventSystem->DispatchAllEvents();

	graphicsEngine->Render();

	THEBE_PROFILE_END_FRAME;
}

void ChineseCheckersCanvas::OnSize(wxSizeEvent& event)
{
	wxSize windowSize = event.GetSize();
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->Resize(windowSize.x, windowSize.y);
}

CollisionObject* ChineseCheckersCanvas::PickCollisionObject(const wxPoint& mousePoint)
{
	CollisionObject* collisionObject = nullptr;

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CollisionSystem* collisionSystem = graphicsEngine->GetCollisionSystem();

	Vector2 screenCoords(double(mousePoint.x), double(mousePoint.y));
	Ray ray;
	if (graphicsEngine->CalcPickingRay(screenCoords, ray))
	{
		Vector3 unitSurfaceNormal;
		collisionSystem->RayCast(ray, collisionObject, unitSurfaceNormal);
	}

	return collisionObject;
}

void ChineseCheckersCanvas::OnMouseMotion(wxMouseEvent& event)
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CollisionSystem* collisionSystem = graphicsEngine->GetCollisionSystem();

	auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
	if (!scene)
		return;

	//...
}

void ChineseCheckersCanvas::OnMouseLeftClick(wxMouseEvent& event)
{
	// TODO: Click and drag to rotate board...
}

void ChineseCheckersCanvas::OnMouseRightClick(wxMouseEvent& event)
{
	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (!collisionObject)
		return;

	HumanClient* humanClient = wxGetApp().GetHumanClient();
	if (!humanClient)
		return;

	ChineseCheckers::Graph* graph = humanClient->GetGraph();
	if (!graph)
		return;

	int nodeIndex = (int)collisionObject->GetUserData();
	if (this->moveSequence.Extend(nodeIndex, graph, humanClient->GetColor()))
		this->UpdateRings();

	this->UpdateRings();
}

void ChineseCheckersCanvas::OnMouseMiddleClick(wxMouseEvent& event)
{
	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (!collisionObject)
		return;

	HumanClient* humanClient = wxGetApp().GetHumanClient();
	if (!humanClient)
		return;

	ChineseCheckers::Graph* graph = humanClient->GetGraph();
	if (!graph)
		return;

	int i = (int)collisionObject->GetUserData();

	if (this->moveSequence.nodeIndexArray.size() > 0 && this->moveSequence.nodeIndexArray[this->moveSequence.nodeIndexArray.size() - 1] == i)
		humanClient->MakeMove(this->moveSequence);
	
	this->moveSequence.nodeIndexArray.clear();

	this->UpdateRings();
}

void ChineseCheckersCanvas::SetDebugDraw(bool debugDraw)
{
	this->debugDraw = debugDraw;
}

bool ChineseCheckersCanvas::GetDebugDraw() const
{
	return this->debugDraw;
}

void ChineseCheckersCanvas::UpdateRings()
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CollisionSystem* collisionSystem = graphicsEngine->GetCollisionSystem();

	auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
	if (!scene)
		return;

	int i = 0;
	while (true)
	{
		MeshInstance* ringInstance = dynamic_cast<MeshInstance*>(scene->GetRootSpace()->FindSpaceByName(std::format("ring{}", i)));
		if (!ringInstance)
			break;

		ringInstance->SetFlags(ringInstance->GetFlags() & ~THEBE_RENDER_OBJECT_FLAG_VISIBLE);
		i++;
	}

	HumanClient* humanClient = wxGetApp().GetHumanClient();
	if (!humanClient)
		return;

	ChineseCheckers::Graph* graph = humanClient->GetGraph();
	if (!graph)
		return;

	for (i = 0; i < (int)this->moveSequence.nodeIndexArray.size(); i++)
	{
		int j = this->moveSequence.nodeIndexArray[i];
		Node* node = dynamic_cast<Node*>(graph->GetNodeArray()[j]);
		THEBE_ASSERT_FATAL(node != nullptr);

		MeshInstance* ringInstance = dynamic_cast<MeshInstance*>(scene->GetRootSpace()->FindSpaceByName(std::format("ring{}", i)));
		THEBE_ASSERT_FATAL(ringInstance != nullptr);

		ringInstance->SetFlags(ringInstance->GetFlags() | THEBE_RENDER_OBJECT_FLAG_VISIBLE);

		Transform objectToWorld;
		objectToWorld.SetIdentity();
		objectToWorld.translation = node->GetLocation3D() + Vector3(0.0, 2.5, 0.0);
		ringInstance->SetChildToParentTransform(objectToWorld);
	}
}