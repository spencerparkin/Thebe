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

const AVLTreeNode* AVLTree::GetRootNode() const
{
	return this->rootNode;
}

AVLTreeNode* AVLTree::FindNode(const AVLTreeKey* givenKey)
{
	AVLTreeNode* node = this->rootNode;
	while (node && node->GetKey()->IsNotEqualto(givenKey))
	{
		if (givenKey->IsLessThan(node->GetKey()))
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
			if (givenNode->GetKey()->IsEqualto(node->GetKey()))
				return false;

			if (givenNode->GetKey()->IsLessThan(node->GetKey()))
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
		this->RebalanceAtNode(givenNode);
	}
	else
	{
		this->rootNode = givenNode;
		givenNode->parentNode = nullptr;
	}

	givenNode->tree = this;
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
		// This choice is arbitrary.
#if true
		AVLTreeNode* node = givenNode->Predecessor();
#else
		AVLTreeNode* node = givenNode->Successor();
#endif
		this->RemoveNode(node);
		givenNode->ReplaceWith(node, true);
		node->UpdateBalanceFactor();
	}
	else
	{
		AVLTreeNode* node = givenNode->leftNode ? givenNode->leftNode : givenNode->rightNode;
		givenNode->ReplaceWith(node, false);
		this->RebalanceAtNode(givenNode->parentNode);
		this->nodeCount--;
	}
	
	givenNode->parentNode = nullptr;
	givenNode->leftNode = nullptr;
	givenNode->rightNode = nullptr;
	givenNode->tree = nullptr;
	if (deleteNode)
		delete givenNode;

	return true;
}

void AVLTree::RebalanceAtNode(AVLTreeNode* node)
{
	while (node)
	{
		node->UpdateBalanceFactor();
		AVLTreeNode* parentNode = node->parentNode;
		node->BalanceSubtree();
		node = parentNode;
	}
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
	{
		this->rootNode->UpdateBalanceFactorsRecursively();

		return this->rootNode->IsAVLTree();
	}

	return true;
}

bool AVLTree::IsBinaryTree() const
{
	if (this->rootNode)
		return this->rootNode->IsBinaryTree();

	return true;
}

int AVLTree::GetNodeCount(bool walkTree /*= false*/) const
{
	if (walkTree)
	{
		int count = 0;

		if (this->rootNode)
		{
			this->rootNode->Traverse([&count](const AVLTreeNode* node) -> bool
				{
					count++;
					return true;
				});
		}

		return count;
	}

	return this->nodeCount;
}

bool AVLTree::Traverse(std::function<bool(const AVLTreeNode*)> callback) const
{
	if (this->rootNode)
		return this->rootNode->Traverse(callback);

	return true;
}

//------------------------------------ AVLTreeNode ------------------------------------

AVLTreeNode::AVLTreeNode()
{
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

const AVLTreeNode* AVLTreeNode::GetLeftNode() const
{
	return this->leftNode;
}

const AVLTreeNode* AVLTreeNode::GetRightNode() const
{
	return this->rightNode;
}

const AVLTreeNode* AVLTreeNode::GetParentNode() const
{
	return this->parentNode;
}

AVLTreeNode* AVLTreeNode::Find(const AVLTreeKey* givenKey)
{
	if (givenKey->IsLessThan(this->GetKey()))
	{
		if (this->leftNode)
			return this->leftNode->Find(givenKey);
	}
	else if (givenKey->IsGreaterThan(this->GetKey()))
	{
		if (this->rightNode)
			if (this->rightNode->Find(givenKey));
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

	oldRootNode->UpdateBalanceFactor();
	newRootNode->UpdateBalanceFactor();

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

	oldRootNode->UpdateBalanceFactor();
	newRootNode->UpdateBalanceFactor();

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
	// Here we assume that the balance factors and depth measurement of our children are already correct.

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

void AVLTreeNode::UpdateBalanceFactorsRecursively()
{
	if (this->leftNode)
		this->leftNode->UpdateBalanceFactorsRecursively();

	if (this->rightNode)
		this->rightNode->UpdateBalanceFactorsRecursively();

	this->UpdateBalanceFactor();
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

const AVLTreeNode* AVLTreeNode::Predecessor() const
{
	return const_cast<AVLTreeNode*>(this)->Predecessor();
}

const AVLTreeNode* AVLTreeNode::Successor() const
{
	return const_cast<AVLTreeNode*>(this)->Successor();
}

void AVLTreeNode::ReplaceWith(AVLTreeNode* node, bool adopt)
{
	if (node == this)
		return;

	if (node)
	{
		node->SetKey(this->GetKey());

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

	if (this->parentNode)
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
	if (this->balanceFactor < -1 || this->balanceFactor > 1)
		return false;

	if (this->leftNode && !this->leftNode->IsAVLTree())
		return false;
	
	if (this->rightNode && !this->rightNode->IsAVLTree())
		return false;

	return true;
}

bool AVLTreeNode::IsBinaryTree() const
{
	const AVLTreeKey* largestKey = nullptr;
	const AVLTreeKey* smallestKey = nullptr;

	if (this->leftNode)
	{
		this->leftNode->Traverse([&largestKey](const AVLTreeNode* node) -> bool
			{
				if (!largestKey || node->GetKey()->IsGreaterThan(largestKey))
					largestKey = node->GetKey();
				return true;
			});
	}

	if (this->rightNode)
	{
		this->rightNode->Traverse([&smallestKey](const AVLTreeNode* node) -> bool
			{
				if (!smallestKey || node->GetKey()->IsLessThan(smallestKey))
					smallestKey = node->GetKey();
				return true;
			});
	}

	if (largestKey && !largestKey->IsLessThan(this->GetKey()))
		return false;

	if (smallestKey && !smallestKey->IsGreaterThan(this->GetKey()))
		return false;

	if (this->leftNode && !this->leftNode->IsBinaryTree())
		return false;

	if (this->rightNode && !this->rightNode->IsBinaryTree())
		return false;

	return true;
}

bool AVLTreeNode::Traverse(std::function<bool(const AVLTreeNode*)> callback) const
{
	if (this->leftNode)
		if (!this->leftNode->Traverse(callback))
			return false;

	if (!callback(this))
		return false;

	if (this->rightNode)
		if (!this->rightNode->Traverse(callback))
			return false;

	return true;
}