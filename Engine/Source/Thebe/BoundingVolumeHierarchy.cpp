#include "Thebe/BoundingVolumeHierarchy.h"
#include "Thebe/Log.h"
#include <algorithm>

using namespace Thebe;

//---------------------------------------- BVHTree ----------------------------------------

BVHTree::BVHTree()
{
}

/*virtual*/ BVHTree::~BVHTree()
{
}

void BVHTree::SetWorldBox(const AxisAlignedBoundingBox& worldBox)
{
	this->worldBox = worldBox;
}

const AxisAlignedBoundingBox& BVHTree::GetWorldBox() const
{
	return this->worldBox;
}

bool BVHTree::AddObject(BVHObject* object)
{
	if (!object)
		return false;

	Reference<BVHNode> node;
	if (object->GetNode(node))
	{
		THEBE_LOG("Given object is already in a tree.");
		return false;
	}

	if (!this->rootNode.Get())
	{
		this->rootNode.Set(new BVHNode());
		this->rootNode->worldBox = this->worldBox;
	}

	object->SetNode(this->rootNode);
	
	return object->UpdateBVHLocation();
}

bool BVHTree::RemoveObject(BVHObject* object)
{
	if (!object)
		return false;

	Reference<BVHNode> node;
	if (!object->GetNode(node))
	{
		THEBE_LOG("Given object is not in a tree.");
		return false;
	}

	if (node->treeHandle != this->GetHandle())
	{
		THEBE_LOG("Given object is not in this tree.");
		return false;
	}

	node->RemoveObject(object);
	object->SetNode(nullptr);

	return true;
}

void BVHTree::RemoveAllObjects()
{
	this->rootNode = nullptr;
}

BVHObject* BVHTree::FindNearestObjectHitByRay(const Ray& ray, Vector3& unitSurfaceNormal)
{
	if (!this->rootNode.Get())
		return nullptr;

	if (!ray.CastAgainst(this->rootNode->GetWorldBox(), this->rootNode->rayHitInterval))
		return nullptr;

	return this->rootNode->FindNearestObjectHitByRay(ray, unitSurfaceNormal);
}

void BVHTree::FindObjects(const AxisAlignedBoundingBox& worldBox, std::list<BVHObject*>& objectList)
{
	objectList.clear();
	if (!this->rootNode.Get())
		return;

	std::list<BVHNode*> queue;
	queue.push_back(this->rootNode.Get());
	while (queue.size() > 0)
	{
		BVHNode* node = *queue.begin();
		queue.pop_front();

		AxisAlignedBoundingBox box;
		if (!box.Intersect(worldBox, node->worldBox))
			continue;

		for (auto object : node->objectList)
			if (box.Intersect(worldBox, object->GetWorldBoundingBox()))
				objectList.push_back(object);

		for (auto& childNode : node->childNodeArray)
			queue.push_back(childNode.Get());
	}
}

//---------------------------------------- BVHNode ----------------------------------------

BVHNode::BVHNode()
{
	this->parentNode = nullptr;
	this->treeHandle = THEBE_INVALID_REF_HANDLE;
}

/*virtual*/ BVHNode::~BVHNode()
{
	this->childNodeArray.clear();
}

RefHandle BVHNode::GetTreeHandle() const
{
	return this->treeHandle;
}

const AxisAlignedBoundingBox& BVHNode::GetWorldBox() const
{
	return this->worldBox;
}

void BVHNode::RemoveObject(BVHObject* object)
{
	for (std::list<Reference<BVHObject>>::iterator iter = this->objectList.begin(); iter != this->objectList.end(); iter++)
	{
		if ((*iter).Get() == object)
		{
			this->objectList.erase(iter);
			break;
		}
	}
}

void BVHNode::AddObject(BVHObject* object)
{
	this->objectList.push_back(object);
}

BVHObject* BVHNode::FindNearestObjectHitByRay(const Ray& ray, Vector3& unitSurfaceNormal)
{
	// Here we operate on the principles that 1) we already know that the given ray hits or originates
	// in this node's box/space, and 2) that this node's children form a pair-wise disjoint set of boxes.

	// First, gather the child nodes hit by or containing the origin of the given ray.
	std::vector<BVHNode*> hitChildNodeArray;
	for (BVHNode* node : this->childNodeArray)
		if (ray.CastAgainst(node->GetWorldBox(), node->rayHitInterval))
			hitChildNodeArray.push_back(node);

	// Next, sort the sub-spaces by nearest to farthest hit.
	std::sort(hitChildNodeArray.begin(), hitChildNodeArray.end(), [](const BVHNode* nodeA, const BVHNode* nodeB) -> bool
		{
			return nodeA->rayHitInterval.A < nodeB->rayHitInterval.A;
		});

	// Now process the nodes in the sorted order.  By virtue of the order, if we get a hit,
	// this allows us to early-out of the loop, there-by culling branches of the tree.
	BVHObject* nearestHitObject = nullptr;
	for (BVHNode* node : hitChildNodeArray)
	{
		nearestHitObject = node->FindNearestObjectHitByRay(ray, unitSurfaceNormal);
		if (nearestHitObject)
			break;
	}

	// In an attempt to minimize the number of calls we'll have to make
	// to cast a ray against an actual object, gather the objects in this
	// node's space who's bounds are hit and sort them nearest to farthest.
	std::vector<BVHObject*> hitObjectBoundsArray;
	for (BVHObject* object : this->objectList)
		if (ray.CastAgainst(object->GetWorldBoundingBox(), object->rayBoundsHitInterval))
			hitObjectBoundsArray.push_back(object);
	std::sort(hitObjectBoundsArray.begin(), hitObjectBoundsArray.end(), [](const BVHObject* objectA, const BVHObject* objectB) -> bool
		{
			return objectA->rayBoundsHitInterval.A < objectB->rayBoundsHitInterval.A;
		});

	// Lastly, compare our nearest hit object, if any, with all those
	// in the sorted list of objects at this node, keeping the nearest
	// hit as we go along.
	for (BVHObject* object : hitObjectBoundsArray)
	{
		if (nearestHitObject && nearestHitObject->rayObjectHitDistance < object->rayBoundsHitInterval.A)
			break;

		Vector3 tentativeSurfaceNormal;
		if (!object->RayCast(ray, object->rayObjectHitDistance, tentativeSurfaceNormal))
			continue;
			
		if (!nearestHitObject || nearestHitObject->rayObjectHitDistance > object->rayObjectHitDistance)
		{
			nearestHitObject = object;
			unitSurfaceNormal = tentativeSurfaceNormal;
		}
	}

	return nearestHitObject;
}

//---------------------------------------- BVHObject ----------------------------------------

BVHObject::BVHObject()
{
	this->nodeHandle = THEBE_INVALID_REF_HANDLE;
	this->rayObjectHitDistance = 0.0;
}

/*virtual*/ BVHObject::~BVHObject()
{
}

/*virtual*/ bool BVHObject::Cut(const AxisAlignedBoundingBox& box, Reference<BVHObject>& newObject)
{
	return false;
}

bool BVHObject::GetNode(Reference<BVHNode>& node)
{
	Reference<ReferenceCounted> ref;
	if (!HandleManager::Get()->GetObjectFromHandle(this->nodeHandle, ref))
		return false;

	node.SafeSet(ref);
	return node.Get() != nullptr;
}

void BVHObject::SetNode(BVHNode* node)
{
	this->nodeHandle = node ? node->GetHandle() : THEBE_INVALID_REF_HANDLE;
}

bool BVHObject::IsInBVH() const
{
	return this->nodeHandle != THEBE_INVALID_REF_HANDLE;
}

bool BVHObject::UpdateBVHLocation(bool allowCuts /*= false*/)
{
	Reference<BVHNode> node;
	if (!this->GetNode(node))
		return false;

	node->RemoveObject(this);
	this->SetNode(nullptr);

	AxisAlignedBoundingBox worldBoundingBox = this->GetWorldBoundingBox();

	while (node.Get())
	{
		if (!node->worldBox.ContainsBox(worldBoundingBox))
			node = node->parentNode;
		else
			break;
	}
	
	if (!node.Get())
		return false;

	bool wentDeeper = false;
	while (true)
	{
		if (node->childNodeArray.size() == 0)
		{
			auto nodeA = new BVHNode();
			auto nodeB = new BVHNode();

			node->worldBox.Split(nodeA->worldBox, nodeB->worldBox);

			node->childNodeArray.push_back(nodeA);
			node->childNodeArray.push_back(nodeB);

			nodeA->parentNode = node.Get();
			nodeB->parentNode = node.Get();
		}
		
		wentDeeper = false;
		for (auto& childNode : node->childNodeArray)
		{
			if (childNode->worldBox.ContainsBox(worldBoundingBox))
			{
				node = childNode;
				wentDeeper = true;
				break;
			}
		}

		if (wentDeeper)
			continue;

		if (!allowCuts)
			break;

		for (auto& childNode : node->childNodeArray)
		{
			Reference<BVHObject> newObject;
			if (this->Cut(childNode->worldBox, newObject))
			{
				newObject->SetNode(childNode);
				if (!newObject->UpdateBVHLocation(true))
					return false;
			}
		}

		return true;
	}

	node->AddObject(this);
	this->SetNode(node);

	return true;
}