#include "AVLTree.h"

using namespace Thebe;

//------------------------------------ AVLTree ------------------------------------

AVLTree::AVLTree()
{
	this->rootNode = nullptr;
	this->nodeCount = 0;
}

AVLTree::~AVLTree()
{
}

AVLTreeNode* AVLTree::FindNode(int key)
{
	AVLTreeNode* node = this->rootNode;
	while (node && node->key != key)
	{
		if (key < node->key)
			node = node->leftNode;
		else
			node = node->rightNode;
	}

	return node;
}

bool AVLTree::InsertNode(AVLTreeNode* givenNode)
{
	if (givenNode->tree)
		return false;

	if (this->rootNode)
	{
		AVLTreeNode* node = this->rootNode;
		for (;;)
		{
			if (givenNode->key == node->key)
				return false;

			if (givenNode->key < node->key)
			{
				if (node->leftNode)
					node = node->leftNode;
				else
				{
					node->leftNode = givenNode;
					break;
				}
			}
			else
			{
				if (node->rightNode)
					node = node->rightNode;
				else
				{
					node->rightNode = givenNode;
					break;
				}
			}
		}

		givenNode->parentNode = node;
		
		while (node)
		{
			node->UpdateBalanceFactor();
			node->BalanceSubtree();
			node = node->parentNode;
		}
	}
	else
	{
		this->rootNode = givenNode;
		givenNode->parentNode = nullptr;
	}

	givenNode->tree = this;
	givenNode->leftNode = nullptr;
	givenNode->rightNode = nullptr;
	givenNode->maxDepth = 1;
	givenNode->balanceFactor = 0;
	this->nodeCount++;
	return true;
}

bool AVLTree::RemoveNode(AVLTreeNode* givenNode, bool deleteNode /*= false*/)
{
	if (givenNode->tree != this)
		return false;

	if (givenNode->leftNode && givenNode->rightNode)
	{
		AVLTreeNode* node = nullptr;
		
		// This choice is arbitrary.
#if true
		node = givenNode->Predecessor();
#else
		node = givenNode->Successor();
#endif

		this->RemoveNode(node);
		givenNode->Hijack(node, true);
		node->UpdateBalanceFactor();
	}
	else
	{
		AVLTreeNode* node = (givenNode->leftNode ? givenNode->leftNode : givenNode->rightNode);
		givenNode->Hijack(node, false);

		for (node = givenNode->parentNode; node; node = node->parentNode)
		{
			node->UpdateBalanceFactor();
			node->BalanceSubtree();
		}
	}

	givenNode->parentNode = nullptr;
	givenNode->leftNode = nullptr;
	givenNode->rightNode = nullptr;
	if (deleteNode)
		delete givenNode;
	this->nodeCount--;
	return true;
}

void AVLTree::Clear(bool deleteNodes /*= false*/)
{
	while (this->rootNode)
	{
		AVLTreeNode* node = this->rootNode;
		this->RemoveNode(this->rootNode);
		if (deleteNodes)
			delete node;
	}
}

AVLTreeNode* AVLTree::Minimum(void)
{
	if (!this->rootNode)
		return nullptr;

	AVLTreeNode* node = this->rootNode;
	while (node->leftNode)
		node = node->leftNode;

	return node;
}

AVLTreeNode* AVLTree::Maximum(void)
{
	if (!this->rootNode)
		return nullptr;

	AVLTreeNode* node = this->rootNode;
	while (node->rightNode)
		node = node->rightNode;

	return node;
}

bool AVLTree::IsAVLTree(void) const
{
	if (this->rootNode)
		return this->rootNode->IsAVLTree();

	return true;
}

int AVLTree::GetNodeCount() const
{
	return this->nodeCount;
}

//------------------------------------ AVLTreeNode ------------------------------------

AVLTreeNode::AVLTreeNode(int key)
{
	this->key = key;
	this->leftNode = nullptr;
	this->rightNode = nullptr;
	this->parentNode = nullptr;
	this->tree = nullptr;
	this->balanceFactor = 0;
	this->maxDepth = 0;
}

AVLTreeNode::~AVLTreeNode()
{
}

AVLTreeNode* AVLTreeNode::Find(int key)
{
	if (key < this->key)
	{
		if (this->leftNode)
			return this->leftNode->Find(key);
	}
	else if (key > this->key)
	{
		if (this->rightNode)
			if (this->rightNode->Find(key));
	}

	return this;
}

bool AVLTreeNode::IsRoot(void) const
{
	return this->parentNode ? false : true;
}


bool AVLTreeNode::IsLeaf(void) const
{
	return !(this->leftNode || this->rightNode);
}

/*
   A < B < C < D               A < B < C < D

		 B                              D
	   /   \                          /
	  A     D         --->           B
		   /                       /   \
		 C                        A     C
*/

bool AVLTreeNode::RotateLeft()
{
	if (!this->rightNode)
		return false;

	AVLTreeNode* newRootNode = this->rightNode;
	AVLTreeNode* oldRootNode = this;

	newRootNode->parentNode = oldRootNode->parentNode;
	if (oldRootNode->parentNode)
	{
		if (oldRootNode->parentNode->leftNode == oldRootNode)
			oldRootNode->parentNode->leftNode = newRootNode;
		else
			oldRootNode->parentNode->rightNode = newRootNode;
	}
	else
		this->tree->rootNode = newRootNode;

	if (newRootNode->leftNode)
		newRootNode->leftNode->parentNode = oldRootNode;
	oldRootNode->rightNode = newRootNode->leftNode;

	newRootNode->leftNode = oldRootNode;
	oldRootNode->parentNode = newRootNode;

	oldRootNode->UpdateBalanceFactorsToRoot();
	return true;
}

/*
   A < B < C < D               A < B < C < D

		 C                        A
	   /   \                        \
	  A     D         --->           C
	   \                           /   \
		 B                        B     D
*/

bool AVLTreeNode::RotateRight()
{
	if (!this->leftNode)
		return false;

	AVLTreeNode* newRootNode = this->leftNode;
	AVLTreeNode* oldRootNode = this;

	newRootNode->parentNode = oldRootNode->parentNode;
	if (oldRootNode->parentNode)
	{
		if (oldRootNode->parentNode->leftNode == oldRootNode)
			oldRootNode->parentNode->leftNode = newRootNode;
		else
			oldRootNode->parentNode->rightNode = newRootNode;
	}
	else
		this->tree->rootNode = newRootNode;

	if (newRootNode->rightNode)
		newRootNode->rightNode->parentNode = oldRootNode;
	oldRootNode->leftNode = newRootNode->rightNode;

	newRootNode->rightNode = oldRootNode;
	oldRootNode->parentNode = newRootNode;

	oldRootNode->UpdateBalanceFactorsToRoot();
	return true;
}

void AVLTreeNode::BalanceSubtree()
{
	if (this->balanceFactor > 1)
	{
		if (this->rightNode && this->rightNode->balanceFactor < 0)
			this->rightNode->RotateRight();

		this->RotateLeft();
	}
	else if (this->balanceFactor < -1)
	{
		if (this->leftNode && this->leftNode->balanceFactor > 0)
			this->leftNode->RotateLeft();

		this->RotateRight();
	}
}

void AVLTreeNode::UpdateBalanceFactor(void)
{
	if (this->leftNode && this->rightNode)
	{
		if (this->leftNode->maxDepth > this->rightNode->maxDepth)
			this->maxDepth = this->leftNode->maxDepth + 1;
		else
			this->maxDepth = this->rightNode->maxDepth + 1;

		this->balanceFactor = this->rightNode->maxDepth - this->leftNode->maxDepth;
	}
	else if (this->leftNode)
	{
		this->maxDepth = this->leftNode->maxDepth + 1;
		this->balanceFactor = -this->leftNode->maxDepth;
	}
	else if (this->rightNode)
	{
		this->maxDepth = this->rightNode->maxDepth + 1;
		this->balanceFactor = this->rightNode->maxDepth;
	}
	else
	{
		this->maxDepth = 1;
		this->balanceFactor = 0;
	}
}

void AVLTreeNode::UpdateBalanceFactorsToRoot()
{
	AVLTreeNode* node = this;
	while (node)
	{
		node->UpdateBalanceFactor();
		node = node->parentNode;
	}
}

AVLTreeNode* AVLTreeNode::Predecessor(void)
{
	AVLTreeNode* nodeA = nullptr;

	if (!this->leftNode)
	{
		AVLTreeNode* nodeB = nullptr;
		nodeA = this;
		do
		{
			nodeB = nodeA;
			nodeA = nodeA->parentNode;
		} while (nodeA && nodeA->rightNode != nodeB);
	}
	else
	{
		nodeA = leftNode;
		while (nodeA->rightNode)
			nodeA = nodeA->rightNode;
	}

	return nodeA;
}

AVLTreeNode* AVLTreeNode::Successor(void)
{
	AVLTreeNode* nodeA = nullptr;

	if (!this->rightNode)
	{
		AVLTreeNode* nodeB = nullptr;
		nodeA = this;
		do
		{
			nodeB = nodeA;
			nodeA = nodeA->parentNode;
		} while (nodeA && nodeA->leftNode != nodeB);
	}
	else
	{
		nodeA = rightNode;
		while (nodeA->leftNode)
			nodeA = nodeA->leftNode;
	}

	return nodeA;
}

void AVLTreeNode::Hijack(AVLTreeNode* node, bool adopt)
{
	if (node == this)
		return;

	if (node)
	{
		if (adopt)
		{
			node->leftNode = this->leftNode;
			node->rightNode = this->rightNode;

			if (this->leftNode)
				this->leftNode->parentNode = node;
			if (this->rightNode)
				this->rightNode->parentNode = node;
		}

		node->parentNode = parentNode;
	}

	if (parentNode)
	{
		if (this->parentNode->leftNode == this)
			this->parentNode->leftNode = node;
		else
			this->parentNode->rightNode = node;
	}
	else
		this->tree->rootNode = node;
}

bool AVLTreeNode::IsAVLTree(void) const
{
	return this->balanceFactor >= -1 && this->balanceFactor <= 1 &&
		(!this->leftNode || this->leftNode->IsAVLTree()) &&
		(!this->rightNode || this->rightNode->IsAVLTree());
}