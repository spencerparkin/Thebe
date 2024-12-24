#include "Thebe/BoundingVolumeHierarchy.h"
#include "Thebe/Log.h"

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

//---------------------------------------- BVHObject ----------------------------------------

BVHObject::BVHObject()
{
	this->nodeHandle = THEBE_INVALID_REF_HANDLE;
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

bool BVHObject::UpdateBVHLocation(bool allowCuts /*= false*/)
{
	Reference<BVHNode> node;
	if (!this->GetNode(node))
		return false;

	node->RemoveObject(this);
	this->SetNode(nullptr);

	AxisAlignedBoundingBox worldBoundingBox = this->GetWorldBoundingBox();

	while (node.Get())
		if (!node->worldBox.ContainsBox(worldBoundingBox))
			node = node->parentNode;
	
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