#pragma once

#include "Thebe/Common.h"

namespace Thebe
{
	class LinkedListNode;

	/**
	 * These are doubly-linked lists.
	 */
	class THEBE_API LinkedList
	{
	public:
		LinkedList();
		virtual ~LinkedList();

		/**
		 * Insert the given node into this linked list.  Failure can
		 * occur if the given node is already in a list.
		 * 
		 * @param[in,out] givenNode This is the node to be inserted.
		 * @param[in,out] beforeNode This is the node of this list to preceed the given node.  If not given, the given node is appended to the list.
		 * @return True is returned on success; false, otherwise.
		 */
		bool InsertNodeAfter(LinkedListNode* givenNode, LinkedListNode* beforeNode = nullptr);

		/**
		 * Insert the given node into this linked list.  Failure can
		 * occur if the given node is already in a list.
		 * 
		 * @param[in,out] givenNode This is the node to be inserted.
		 * @param[in,out] afterNode This is the node of this list to succeed the given node.  If not given, the given node is prepended to the list.
		 * @return True is returned on success; false, otherwise.
		 */
		bool InsertNodeBefore(LinkedListNode* givenNode, LinkedListNode* afterNode = nullptr);

		/**
		 * Remove the given node from this list.  Failure can occur
		 * if the given node is not a member of this list.
		 * 
		 * @param[in,out] givenNode This is the node to be removed.
		 * @param[in] deleteNode If true, the given node is also deleted.
		 * @param True is returned on success; false, otherwise.
		 */
		bool RemoveNode(LinkedListNode* givenNode, bool deleteNode = false);

		/**
		 * Remove all nodes from this linked list.
		 * 
		 * @param[in] deleteNodes If true, the delete operator is used to free each node that is removed.
		 */
		void Clear(bool deleteNodes = false);

		/**
		 * Return the number of nodes in this list.
		 */
		int GetNodeCount() const;

		LinkedListNode* GetHeadNode();
		LinkedListNode* GetTailNode();

		const LinkedListNode* GetHeadNode() const;
		const LinkedListNode* GetTailNode() const;

	private:
		LinkedListNode* headNode;
		LinkedListNode* tailNode;
		int nodeCount;
	};

	/**
	 * These are members of an instance of the @ref LinkedList class.
	 * Define a derivative in order to provide satilite data.
	 */
	class THEBE_API LinkedListNode
	{
		friend class LinkedList;

	public:
		LinkedListNode();
		virtual ~LinkedListNode();

		void Couple(LinkedListNode* beforeNode, LinkedListNode* afterNode);
		void Decouple();

		LinkedListNode* GetPrevNode();
		LinkedListNode* GetNextNode();

		const LinkedListNode* GetPrevNode() const;
		const LinkedListNode* GetNextNode() const;

	private:
		LinkedList* linkedList;
		LinkedListNode* nextNode;
		LinkedListNode* prevNode;
	};
}