#include "LinkedList.h"

using namespace Thebe;

//---------------------------------- LinkedList ----------------------------------

LinkedList::LinkedList()
{
	this->headNode = nullptr;
	this->tailNode = nullptr;
	this->nodeCount = 0;
}

/*virtual*/ LinkedList::~LinkedList()
{
}

bool LinkedList::InsertNodeAfter(LinkedListNode* givenNode, LinkedListNode* beforeNode /*= nullptr*/)
{
	if (givenNode->linkedList)
		return false;

	if (this->nodeCount == 0)
	{
		this->headNode = givenNode;
		this->tailNode = givenNode;
	}
	else
	{
		if (!beforeNode)
			beforeNode = this->tailNode;

		givenNode->Couple(beforeNode, beforeNode->nextNode);

		if (this->tailNode->nextNode)
			this->tailNode = this->tailNode->nextNode;
	}

	givenNode->linkedList = this;
	this->nodeCount++;
	return true;
}

bool LinkedList::InsertNodeBefore(LinkedListNode* givenNode, LinkedListNode* afterNode /*= nullptr*/)
{
	if (givenNode->linkedList)
		return false;

	if (this->nodeCount == 0)
	{
		this->headNode = givenNode;
		this->tailNode = givenNode;
	}
	else
	{
		if (!afterNode)
			afterNode = this->headNode;

		givenNode->Couple(afterNode->prevNode, afterNode);

		if (this->headNode->prevNode)
			this->headNode = this->headNode->prevNode;
	}

	givenNode->linkedList = this;
	this->nodeCount++;
	return true;
}

bool LinkedList::RemoveNode(LinkedListNode* givenNode, bool deleteNode /*= false*/)
{
	if (givenNode->linkedList != this)
		return false;

	if (this->headNode == givenNode)
		this->headNode = this->headNode->nextNode;

	if (this->tailNode == givenNode)
		this->tailNode = this->tailNode->prevNode;

	givenNode->Decouple();
	givenNode->linkedList = nullptr;
	if (deleteNode)
		delete givenNode;
	
	this->nodeCount--;

	if (this->nodeCount == 0)
	{
		this->headNode = nullptr;
		this->tailNode = nullptr;
	}

	return true;
}

void LinkedList::Clear(bool deleteNodes /*= false*/)
{
	while (this->headNode)
	{
		LinkedListNode* node = this->headNode;
		this->RemoveNode(node);
		if (deleteNodes)
			delete node;
	}
}

int LinkedList::GetNodeCount() const
{
	return this->nodeCount;
}

//---------------------------------- LinkedListNode ----------------------------------

LinkedListNode::LinkedListNode()
{
	this->nextNode = nullptr;
	this->prevNode = nullptr;
	this->linkedList = nullptr;
}

/*virtual*/ LinkedListNode::~LinkedListNode()
{
}

void LinkedListNode::Couple(LinkedListNode* beforeNode, LinkedListNode* afterNode)
{
	this->prevNode = beforeNode;
	this->nextNode = afterNode;

	if (this->nextNode)
		this->nextNode->prevNode = this;

	if(this->prevNode)
		this->prevNode->nextNode = this;
}

void LinkedListNode::Decouple()
{
	if (this->nextNode)
		this->nextNode->prevNode = this->prevNode;

	if (this->prevNode)
		this->prevNode->nextNode = this->nextNode;

	this->nextNode = nullptr;
	this->prevNode = nullptr;
}

const LinkedListNode* LinkedListNode::GetPrevNode() const
{
	return this->prevNode;
}

const LinkedListNode* LinkedListNode::GetNextNode() const
{
	return this->nextNode;
}