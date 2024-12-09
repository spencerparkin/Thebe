#include "TestAVLTree.h"
#include "Thebe/Log.h"
#include <random>
#include <set>

void TestAVLTree()
{
	using namespace Thebe;

	AVLTree tree;
	int numKeys = 1000;

	for (int i = 1; i <= numKeys; i++)
	{
		tree.InsertNode(new Node(i));

		THEBE_ASSERT(tree.IsBinaryTree());
		THEBE_ASSERT(tree.IsAVLTree());
	}

	for (int i = 1; i <= numKeys; i += 2)
	{
		Key key(i);
		Node* node = (Node*)tree.FindNode(&key);
		THEBE_ASSERT(node != nullptr);
		tree.RemoveNode(node, true);

		THEBE_ASSERT(tree.IsBinaryTree());
		THEBE_ASSERT(tree.IsAVLTree());
	}

	tree.Traverse([](const AVLTreeNode* node) -> bool {
			auto key = (const Key*)node->GetKey();
			THEBE_LOG("Key = %d", key->number);
			return true;
		});

	while (tree.GetNodeCount() > 0)
	{
		tree.RemoveNode(tree.GetRootNode(), true);

		THEBE_ASSERT(tree.IsBinaryTree());
		THEBE_ASSERT(tree.IsAVLTree());
	}
}

//------------------------------------- Key -------------------------------------

Key::Key(int number)
{
	this->number = number;
}

/*virtual*/ Key::~Key()
{
}

/*virtual*/ bool Key::IsLessThan(const AVLTreeKey* key) const
{
	return this->number < ((const Key*)key)->number;
}

/*virtual*/ bool Key::IsGreaterThan(const AVLTreeKey* key) const
{
	return this->number > ((const Key*)key)->number;
}

/*virtual*/ bool Key::IsEqualto(const AVLTreeKey* key) const
{
	return this->number == ((const Key*)key)->number;
}

/*virtual*/ bool Key::IsNotEqualto(const AVLTreeKey* key) const
{
	return this->number != ((const Key*)key)->number;
}

//------------------------------------- Node -------------------------------------

Node::Node(int number) : key(number)
{
}

/*virtual*/ Node::~Node()
{
}

/*virtual*/ const Thebe::AVLTreeKey* Node::GetKey() const
{
	return &this->key;
}

/*virtual*/ void Node::SetKey(const Thebe::AVLTreeKey* givenKey)
{
	this->key.number = ((const Key*)givenKey)->number;
}