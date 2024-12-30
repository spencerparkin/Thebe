#include "Thebe/CollisionSystem.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Log.h"

using namespace Thebe;

//--------------------------------- CollisionSystem ---------------------------------

CollisionSystem::CollisionSystem()
{
	this->boxTree.Set(new BVHTree());
}

/*virtual*/ CollisionSystem::~CollisionSystem()
{
	this->boxTree = nullptr;
}

void CollisionSystem::SetWorldBox(const AxisAlignedBoundingBox& worldBox)
{
	this->boxTree->SetWorldBox(worldBox);
}

const AxisAlignedBoundingBox& CollisionSystem::GetWorldBox() const
{
	return this->boxTree->GetWorldBox();
}

bool CollisionSystem::TrackObject(CollisionObject* collisionObject)
{
	if (!collisionObject)
		return false;

	if (this->collisionObjectMap.find(collisionObject->GetHandle()) != this->collisionObjectMap.end())
	{
		THEBE_LOG("Collision object already tracked.");
		return false;
	}

	if (!this->boxTree->AddObject(collisionObject))
	{
		THEBE_LOG("Failed to add BVH object to the tree.");
		return false;
	}

	this->collisionObjectMap.insert(std::pair(collisionObject->GetHandle(), collisionObject));

	return true;
}

bool CollisionSystem::UntrackObject(CollisionObject* collisionObject)
{
	if (!collisionObject)
		return false;

	if (this->collisionObjectMap.find(collisionObject->GetHandle()) == this->collisionObjectMap.end())
	{
		THEBE_LOG("Collision object not tracked.  Can't untrack it.");
		return false;
	}

	if (!this->boxTree->RemoveObject(collisionObject))
	{
		THEBE_LOG("Failed to remove BVH object from the tree.");
		return false;
	}

	this->collisionObjectMap.erase(collisionObject->GetHandle());

	return true;
}

void CollisionSystem::UntrackAllObjects()
{
	this->collisionObjectMap.clear();
}

void CollisionSystem::FindAllCollisions(CollisionObject* collisionObject, std::vector<Reference<Collision>>& collisionArray)
{
	collisionArray.clear();

	// Peform the broad phase of collision detection.
	AxisAlignedBoundingBox worldBoundingBox = collisionObject->GetWorldBoundingBox();
	std::list<BVHObject*> objectList;
	this->boxTree->FindObjects(worldBoundingBox, objectList);

	// Now perform the narrow phase of collision detection.
	for (auto object : objectList)
	{
		auto otherCollisionObject = dynamic_cast<CollisionObject*>(object);
		if (otherCollisionObject == collisionObject)
			continue;

		Reference<Collision> collision;

		std::string key = this->MakeCollisionCacheKey(collisionObject, otherCollisionObject);
		auto pair = this->collisionCacheMap.find(key);
		if (pair != this->collisionCacheMap.end())
		{
			collision = pair->second;
			if (!collision->StillValid())
			{
				this->collisionCacheMap.erase(pair);
				collision = nullptr;
				pair = this->collisionCacheMap.end();
			}
		}

		if (!collision.Get())
		{
			if (GJKShape::Intersect(collisionObject->GetShape(), otherCollisionObject->GetShape()))
			{
				collision.Set(new Collision());
				collision->validFrameA = collisionObject->GetFrameWhenLastMoved();
				collision->validFrameB = otherCollisionObject->GetFrameWhenLastMoved();
				collision->objectA = collisionObject;
				collision->objectB = otherCollisionObject;
				// TODO: How do we calculate the separation delta?!  The GJK algorithm might contain this information near the end.
			}
		}

		if (collision.Get())
		{
			collisionArray.push_back(collision);

			if (pair == this->collisionCacheMap.end())
				this->collisionCacheMap.insert(std::pair(key, collision));
		}
	}
}

std::string CollisionSystem::MakeCollisionCacheKey(const CollisionObject* objectA, const CollisionObject* objectB)
{
	RefHandle handleA = objectA->GetHandle();
	RefHandle handleB = objectB->GetHandle();

	if (handleA < handleB)
		return std::format("{}_{}", handleA, handleB);
	else
		return std::format("{}_{}", handleB, handleA);
}

void CollisionSystem::DebugDraw(DynamicLineRenderer* lineRenderer, uint32_t& lineOffset) const
{
	for (auto pair : this->collisionObjectMap)
	{
		const CollisionObject* collisionObject = pair.second;
		collisionObject->DebugDraw(lineRenderer, lineOffset);
	}
}

//--------------------------------- CollisionSystem::Collision ---------------------------------

CollisionSystem::Collision::Collision()
{
	this->validFrameA = -1;
	this->validFrameB = -1;
}

/*virtual*/ CollisionSystem::Collision::~Collision()
{
}

bool CollisionSystem::Collision::StillValid() const
{
	if (this->validFrameA != this->objectA->GetFrameWhenLastMoved())
		return false;

	if (this->validFrameB != this->objectB->GetFrameWhenLastMoved())
		return false;

	return true;
}