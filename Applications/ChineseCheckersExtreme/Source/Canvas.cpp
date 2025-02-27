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
#include "BoardCam.h"
#include <wx/utils.h>

using namespace Thebe;

ChineseCheckersCanvas::ChineseCheckersCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &ChineseCheckersCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &ChineseCheckersCanvas::OnSize, this);
	this->Bind(wxEVT_MOTION, &ChineseCheckersCanvas::OnMouseMotion, this);
	this->Bind(wxEVT_LEFT_DOWN, &ChineseCheckersCanvas::OnMouseLeftDown, this);
	this->Bind(wxEVT_LEFT_UP, &ChineseCheckersCanvas::OnMouseLeftUp, this);
	this->Bind(wxEVT_RIGHT_DOWN, &ChineseCheckersCanvas::OnMouseRightDown, this);
	this->Bind(wxEVT_MIDDLE_DOWN, &ChineseCheckersCanvas::OnMouseMiddleDown, this);
	this->Bind(wxEVT_MOUSE_CAPTURE_LOST, &ChineseCheckersCanvas::OnMouseCaptureLost, this);
	this->Bind(wxEVT_MOUSEWHEEL, &ChineseCheckersCanvas::OnMouseWheelMoved, this);

	this->debugDraw = false;
	this->mouseDragging = false;
}

/*virtual*/ ChineseCheckersCanvas::~ChineseCheckersCanvas()
{
}

#if defined THEBE_USE_IMGUI
/*virtual*/ bool ChineseCheckersCanvas::MSWHandleMessage(WXLRESULT* result, WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();

	*result = graphicsEngine->GetImGuiManager()->HandleWindowsMessage(this->GetHWND(), message, wParam, lParam);
	if (*result != 0)
		return true;

	return wxWindow::MSWHandleMessage(result, message, wParam, lParam);
}
#endif //THEBE_USE_IMGUI

void ChineseCheckersCanvas::OnPaint(wxPaintEvent& event)
{
	THEBE_PROFILE_BEGIN_FRAME;

	wxGetApp().GetController()->Update();

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CollisionSystem* collisionSystem = graphicsEngine->GetCollisionSystem();
	PhysicsSystem* physicsSystem = graphicsEngine->GetPhysicsSystem();
	EventSystem* eventSystem = graphicsEngine->GetEventSystem();
	CameraSystem* cameraSystem = graphicsEngine->GetCameraSystem();

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

	cameraSystem->Update(deltaTimeSeconds);

	THEBE_PROFILE_END_FRAME;
}

void ChineseCheckersCanvas::OnSize(wxSizeEvent& event)
{
	wxSize windowSize = event.GetSize();
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->Resize(windowSize.x, windowSize.y);
}

ChineseCheckers::Node* ChineseCheckersCanvas::PickNode(const wxPoint& mousePoint)
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

	if (!collisionObject)
		return nullptr;

	auto node = reinterpret_cast<ChineseCheckers::Node*>(collisionObject->GetUserData());
	if (!node)
	{
		HumanClient* humanClient = wxGetApp().GetHumanClient();
		if (humanClient)
		{
			ChineseCheckers::Graph* graph = humanClient->GetGraph();
			if (graph)
			{
				for (int i = 0; i < (int)graph->GetNodeArray().size(); i++)
				{
					std::shared_ptr<ChineseCheckers::Marble> nativeMarble = graph->GetNodeArray()[i]->GetOccupant();
					auto marble = dynamic_cast<Marble*>(nativeMarble.get());
					if (marble && marble->collisionObjectHandle == collisionObject->GetHandle())
						node = graph->GetNodeArray()[i];
				}
			}
		}
	}

	return node;
}

void ChineseCheckersCanvas::OnMouseMotion(wxMouseEvent& event)
{
	if (!this->mouseDragging)
		return;

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CameraSystem* cameraSystem = graphicsEngine->GetCameraSystem();
	auto boardCam = dynamic_cast<BoardCam*>(cameraSystem->GetControllerByName("board_cam"));
	if (!boardCam)
		return;

	this->mousePointB = event.GetPosition();

	Vector2 screenCoordsA(double(this->mousePointA.x), double(this->mousePointA.y));
	Ray rayA;
	if (!graphicsEngine->CalcPickingRay(screenCoordsA, rayA))
		return;

	Vector2 screenCoordsB(double(this->mousePointB.x), double(this->mousePointB.y));
	Ray rayB;
	if (!graphicsEngine->CalcPickingRay(screenCoordsB, rayB))
		return;

	Plane xzPlane(Vector3::Zero(), Vector3::YAxis());
	Vector3 worldPointA = rayA.CalculatePoint(rayA.CastAgainst(xzPlane));
	Vector3 worldPointB = rayB.CalculatePoint(rayB.CastAgainst(xzPlane));

	if (wxGetKeyState(wxKeyCode::WXK_CONTROL))
	{
		const Vector3& focalPoint = boardCam->GetDynamicParams().focalPoint;
		Vector3 vectorA = worldPointA - focalPoint;
		Vector3 vectorB = worldPointB - focalPoint;
		double angleDelta = vectorA.Normalized().AngleBetween(vectorB.Normalized());
		double det = Vector3::YAxis().Dot(vectorA.Cross(vectorB));
		angleDelta *= THEBE_SIGN(det);
		boardCam->ApplyPivot(angleDelta);
	}
	else
	{
		Vector3 delta = worldPointA - worldPointB;
		boardCam->ApplyStrafe(delta);
	}

	this->mousePointA = this->mousePointB;
}

void ChineseCheckersCanvas::OnMouseLeftDown(wxMouseEvent& event)
{
	this->SetFocus();	// Without this, the ImGui stuff can't get keyboard events.

	this->mousePointA = event.GetPosition();
	this->mouseDragging = true;
	this->CaptureMouse();
}

void ChineseCheckersCanvas::OnMouseLeftUp(wxMouseEvent& event)
{
	if (this->mouseDragging)
	{
		this->mouseDragging = false;
		this->ReleaseMouse();
	}
}

void ChineseCheckersCanvas::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	this->mouseDragging = false;
}

void ChineseCheckersCanvas::OnMouseWheelMoved(wxMouseEvent& event)
{
	double wheelChange = double(event.GetWheelRotation() / event.GetWheelDelta());

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	CameraSystem* cameraSystem = graphicsEngine->GetCameraSystem();
	auto boardCam = dynamic_cast<BoardCam*>(cameraSystem->GetControllerByName("board_cam"));
	if (!boardCam)
		return;

	if (wxGetKeyState(wxKeyCode::WXK_CONTROL))
	{
		static double tiltSensativity = THEBE_PI / 64.0;
		double tiltDelta = -wheelChange * tiltSensativity;
		boardCam->ApplyTilt(tiltDelta);
	}
	else
	{
		static double zoomSensativity = 5.0;
		double zoomDelta = -wheelChange * zoomSensativity;
		boardCam->ApplyZoom(zoomDelta);
	}
}

void ChineseCheckersCanvas::OnMouseRightDown(wxMouseEvent& event)
{
	ChineseCheckers::Node* nativeNode = this->PickNode(event.GetPosition());
	if (!nativeNode)
		return;

	HumanClient* humanClient = wxGetApp().GetHumanClient();
	if (!humanClient)
		return;

	ChineseCheckers::Graph* graph = humanClient->GetGraph();
	if (!graph)
		return;

	int nodeIndex = nativeNode->GetOffset();
	if (this->moveSequence.Extend(nodeIndex, graph, humanClient->GetColor()))
		this->UpdateRings();

	this->UpdateRings();
}

void ChineseCheckersCanvas::OnMouseMiddleDown(wxMouseEvent& event)
{
	ChineseCheckers::Node* nativeNode = this->PickNode(event.GetPosition());
	if (!nativeNode)
		return;

	HumanClient* humanClient = wxGetApp().GetHumanClient();
	if (!humanClient)
		return;

	int nodeIndex = nativeNode->GetOffset();
	if (this->moveSequence.nodeIndexArray.size() > 0 && this->moveSequence.nodeIndexArray[this->moveSequence.nodeIndexArray.size() - 1] == nodeIndex)
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