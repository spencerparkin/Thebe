#include "Canvas.h"
#include "Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/CollisionSystem.h"
#include "Thebe/PhysicsSystem.h"

using namespace Thebe;

ChineseCheckersCanvas::ChineseCheckersCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &ChineseCheckersCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &ChineseCheckersCanvas::OnSize, this);
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