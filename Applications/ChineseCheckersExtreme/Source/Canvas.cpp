#include "Canvas.h"
#include "Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/CollisionSystem.h"
#include "Thebe/PhysicsSystem.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Network/HumanClient.h"

using namespace Thebe;

ChineseCheckersCanvas::ChineseCheckersCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &ChineseCheckersCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &ChineseCheckersCanvas::OnSize, this);
	this->Bind(wxEVT_MOTION, &ChineseCheckersCanvas::OnMouseMotion, this);
	this->Bind(wxEVT_LEFT_DOWN, &ChineseCheckersCanvas::OnMouseLeftClick, this);
	this->Bind(wxEVT_RIGHT_DOWN, &ChineseCheckersCanvas::OnMouseRightClick, this);
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

	int i = (int)this->nodeSequenceArray.size();
	MeshInstance* ringInstance = dynamic_cast<MeshInstance*>(scene->GetRootSpace()->FindSpaceByName(std::format("ring{}", i)));
	if (!ringInstance)
		return;

	ringInstance->SetFlags(ringInstance->GetFlags() & ~THEBE_RENDER_OBJECT_FLAG_VISIBLE);

	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (collisionObject && collisionObject->GetUserData() != 0)
	{
		ringInstance->SetFlags(ringInstance->GetFlags() | THEBE_RENDER_OBJECT_FLAG_VISIBLE);
		const Transform& objectToWorld = collisionObject->GetObjectToWorld();
		ringInstance->SetChildToParentTransform(objectToWorld);
	}
}

void ChineseCheckersCanvas::OnMouseLeftClick(wxMouseEvent& event)
{
	HumanClient* human = wxGetApp().GetHumanClient();
	if (!human)
		return;
	
	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (!collisionObject || !collisionObject->GetUserData())
		return;

	RefHandle handle = (RefHandle)collisionObject->GetUserData();
	Reference<ChineseCheckersGame::Node> nextNode;
	if (!HandleManager::Get()->GetObjectFromHandle(handle, nextNode))
		return;

	if(this->nodeSequenceArray.size() == 0)
	{
		if (!nextNode->occupant)
		{
			// TODO: Maybe play wave saying you must select an occupied node first.
		}
		else if (nextNode->occupant->sourceZoneID != human->GetSourceZoneID())
		{
			// TODO: Maybe play a wave sound saying that you need to click on a platform with your own cube on it.
		}
		else
		{
			this->nodeSequenceArray.push_back(nextNode);
		}
	}
	else
	{
		if (this->nodeSequenceArray.size() == 2 && this->nodeSequenceArray[0]->IsAdjacentTo(this->nodeSequenceArray[1]))
		{
			// TODO: Maybe play wave saying a jump that doesn't hop over another piece can't jump again.
			return;
		}

		ChineseCheckersGame::Node* prevNode = this->nodeSequenceArray[this->nodeSequenceArray.size() - 1];
		std::vector<ChineseCheckersGame::Node*> nodePathArray;
		if (human->GetGame()->FindLegalPath(prevNode, nextNode, nodePathArray))
		{
			for (int i = 1; i < (int)nodePathArray.size(); i++)
				this->nodeSequenceArray.push_back(const_cast<ChineseCheckersGame::Node*>(nodePathArray[i]));
		}
		else
		{
			// TODO: Maybe play a wave sound saying that the move is invalid.
		}
	}

	this->UpdateRings();
}

void ChineseCheckersCanvas::OnMouseRightClick(wxMouseEvent& event)
{
	HumanClient* human = wxGetApp().GetHumanClient();
	if (!human)
		return;

	CollisionObject* collisionObject = this->PickCollisionObject(event.GetPosition());
	if (collisionObject && collisionObject->GetUserData())
	{
		Reference<ChineseCheckersGame::Node> node;
		RefHandle handle = (RefHandle)collisionObject->GetUserData();
		if (HandleManager::Get()->GetObjectFromHandle(handle, node))
		{
			if (this->nodeSequenceArray.size() > 0 && node.Get() == this->nodeSequenceArray[this->nodeSequenceArray.size() - 1])
			{
				human->TakeTurn(this->nodeSequenceArray);
			}
		}
	}
	
	this->nodeSequenceArray.clear();
	this->UpdateRings();
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

	for (i = 0; i < (int)this->nodeSequenceArray.size(); i++)
	{
		MeshInstance* ringInstance = dynamic_cast<MeshInstance*>(scene->GetRootSpace()->FindSpaceByName(std::format("ring{}", i)));
		THEBE_ASSERT_FATAL(ringInstance != nullptr);

		ringInstance->SetFlags(ringInstance->GetFlags() | THEBE_RENDER_OBJECT_FLAG_VISIBLE);

		ChineseCheckersGame::Node* node = this->nodeSequenceArray[i];

		Transform objectToWorld;
		objectToWorld.SetIdentity();
		objectToWorld.translation = node->location;
		ringInstance->SetChildToParentTransform(objectToWorld);
	}
}