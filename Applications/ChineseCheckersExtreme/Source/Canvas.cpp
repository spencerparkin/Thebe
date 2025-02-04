#include "Canvas.h"
#include "Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/CollisionSystem.h"
#include "Thebe/PhysicsSystem.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/CollisionObject.h"

using namespace Thebe;

ChineseCheckersCanvas::ChineseCheckersCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &ChineseCheckersCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &ChineseCheckersCanvas::OnSize, this);
	this->Bind(wxEVT_MOTION, &ChineseCheckersCanvas::OnMouseMotion, this);
	this->Bind(wxEVT_LEFT_DOWN, &ChineseCheckersCanvas::OnMouseLeftClick, this);
	this->Bind(wxEVT_RIGHT_DOWN, &ChineseCheckersCanvas::OnMouseRightClick, this);

	this->debugDraw = false;
}

/*virtual*/ ChineseCheckersCanvas::~ChineseCheckersCanvas()
{
}

void ChineseCheckersCanvas::OnPaint(wxPaintEvent& event)
{
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
	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (!collisionObject)
		return;

	//collisionObject->GetUserData();
	//...
}

void ChineseCheckersCanvas::OnMouseRightClick(wxMouseEvent& event)
{
	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (!collisionObject)
		return;

	//collisionObject->GetUserData();
	//...
}

void ChineseCheckersCanvas::SetDebugDraw(bool debugDraw)
{
	this->debugDraw = debugDraw;
}

bool ChineseCheckersCanvas::GetDebugDraw() const
{
	return this->debugDraw;
}