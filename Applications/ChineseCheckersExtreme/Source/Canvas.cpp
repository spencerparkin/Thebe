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

	this->pickingMode = CHOOSING_SOURCE;
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
#if defined DEBUG_DRAW
		collisionSystem->DebugDraw(lineRenderer);
		physicsSystem->DebugDraw(lineRenderer);
#endif //DEBUG_DRAW
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

void ChineseCheckersCanvas::OnMouseMotion(wxMouseEvent& event)
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CollisionSystem* collisionSystem = graphicsEngine->GetCollisionSystem();

	auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
	if (!scene)
		return;

	wxPoint mousePos = event.GetPosition();
	Vector2 screenCoords(double(mousePos.x), double(mousePos.y));
	Ray ray;
	if (!graphicsEngine->CalcPickingRay(screenCoords, ray))
		return;

	CollisionObject* collisionObject = nullptr;
	Vector3 unitSurfaceNormal;
	collisionSystem->RayCast(ray, collisionObject, unitSurfaceNormal);

	MeshInstance* ringInstance = nullptr;
	switch (this->pickingMode)
	{
	case CHOOSING_SOURCE:
		ringInstance = dynamic_cast<MeshInstance*>(scene->GetRootSpace()->FindSpaceByName("sourceRing"));
		break;
	case CHOOSING_TARGET:
		ringInstance = dynamic_cast<MeshInstance*>(scene->GetRootSpace()->FindSpaceByName("targetRing"));
		break;
	}

	if (!ringInstance)
		return;

	ringInstance->SetFlags(ringInstance->GetFlags() & ~THEBE_RENDER_OBJECT_FLAG_VISIBLE);

	if (collisionObject)
	{
		ringInstance->SetFlags(ringInstance->GetFlags() | THEBE_RENDER_OBJECT_FLAG_VISIBLE);
		const Transform& objectToWorld = collisionObject->GetObjectToWorld();
		ringInstance->SetChildToParentTransform(objectToWorld);
	}
}

void ChineseCheckersCanvas::OnMouseLeftClick(wxMouseEvent& event)
{
	
}