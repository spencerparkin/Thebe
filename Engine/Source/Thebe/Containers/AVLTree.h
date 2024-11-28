#pragma once

#include "Thebe/Common.h"
#include <functional>

namespace Thebe
{
	class AVLTreeNode;

	/**
	 * This class is used to abstract away the notion of a key in a
	 * self-balancing binary tree.  Derivatives must impliment the
	 * comparison operators.
	 */
	class THEBE_API AVLTreeKey
	{
	public:
		virtual bool IsLessThan(const AVLTreeKey* key) const = 0;
		virtual bool IsGreaterThan(const AVLTreeKey* key) const = 0;
		virtual bool IsEqualto(const AVLTreeKey* key) const = 0;
		virtual bool IsNotEqualto(const AVLTreeKey* key) const = 0;
	};

	/**
	 * These are self-balancing binary trees invented by Adelson, Velsky and Landis.
	 */
	class THEBE_API AVLTree
	{
		friend class AVLTreeNode;

	public:
		AVLTree();
		virtual ~AVLTree();

		/**
		 * Find and return the node in this tree with the given key.
		 * 
		 * @param[in] givenKey Nodes of the tree are compared against this key.
		 * @return If found, the node with the given key is returned; null, otherwise.
		 */
		AVLTreeNode* FindNode(const AVLTreeKey* givenKey);

		/**
		 * Insert the given node into this tree.  It is expected that
		 * the key on the given node has already been initialized as
		 * desired.  Failure occurs if a node already exists having the
		 * same key as that of the given node, or if the given node is
		 * already the member of a tree.
		 * 
		 * @param[in,out] givenNode This is the node to be inserted into this tree.
		 * @return True is returned on success; false, otherwise.
		 */
		bool InsertNode(AVLTreeNode* givenNode);

		/**
		 * Remove the given node from this tree.  If the user allocated the node,
		 * then it's up to them to free the memory.  Failure can occur here if
		 * the given node is not a member of this tree.
		 * 
		 * @param[in,out] givenNode This is the node to be removed from this tree.
		 * @param[in] If true, the given node is also deleted.
		 * @REturn True is returned on success; false, otherwise.
		 */
		bool RemoveNode(AVLTreeNode* givenNode, bool deleteNode = false);

		/**
		 * Remove all nodes from this tree.
		 * 
		 * @param[in] deleteNodes If true, the delete operator is used to free each node that is removed.
		 */
		void Clear(bool deleteNodes = false);

		/**
		 * Find and return the node of this tree having the smallest key.
		 */
		AVLTreeNode* Minimum();

		/**
		 * Find and return the node of this tree having the largest key.
		 */
		AVLTreeNode* Maximum();

		/**
		 * For diagnostic purposes, verify that this tree maintains the
		 * properties that every AVL tree is defined to have.
		 */
		bool IsAVLTree() const;

		/**
		 * For diagnostic purposes, verify that this tree maintains the
		 * properties that every binary tree is defined to have.
		 */
		bool IsBinaryTree() const;

		/**
		 * Return the number of nodes in this tree.
		 * 
		 * @param walkTree This is only used for diagnostic purposes to see what the actual count is regardless of the cached count.
		 */
		int GetNodeCount(bool walkTree = false) const;

		/**
		 * Return the root of the binary tree.  Null is returned if the tree is empty.
		 */
		const AVLTreeNode* GetRootNode() const;

		/**
		 * Visit all the nodes of the tree in order.  If the given
		 * callback returns false, the traversal is prematurely
		 * terminated.
		 * 
		 * @param[in] callback This function is called once per node of the tree.
		 * @return True is returned if and only if the traversal explored the tree in its entirety.
		 */
		bool Traverse(std::function<bool(const AVLTreeNode*)> callback) const;

	private:
		void RebalanceAtNode(AVLTreeNode* node);

		AVLTreeNode* rootNode;
		int nodeCount;
	};

	/**
	 * These can be inserted or removed from an instance of the @ref AVLTree class.
	 * Define a derivative of this class in order to provide satilate data.
	 */
	class THEBE_API AVLTreeNode
	{
		friend class AVLTree;

	public:
		AVLTreeNode();
		virtual ~AVLTreeNode();

		virtual const AVLTreeKey* GetKey() const = 0;
		virtual void SetKey(const AVLTreeKey* givenKey) = 0;

		bool RotateLeft();
		bool RotateRight();
		void UpdateBalanceFactor();
		void UpdateBalanceFactorsRecursively();
		void BalanceSubtree();
		AVLTreeNode* Predecessor();
		AVLTreeNode* Successor();
		const AVLTreeNode* Predecessor() const;
		const AVLTreeNode* Successor() const;
		bool IsRoot() const;
		bool IsLeaf() const;
		bool IsAVLTree() const;
		bool IsBinaryTree() const;
		void ReplaceWith(AVLTreeNode* n, bool adopt);
		AVLTreeNode* Find(const AVLTreeKey* givenKey);
		bool Traverse(std::function<bool(const AVLTreeNode*)> callback) const;
		const AVLTreeNode* GetLeftNode() const;
		const AVLTreeNode* GetRightNode() const;
		const AVLTreeNode* GetParentNode() const;

	private:
		AVLTree* tree;
		AVLTreeNode* leftNode;
		AVLTreeNode* rightNode;
		AVLTreeNode* parentNode;
		int balanceFactor;
		int maxDepth;
	};
}