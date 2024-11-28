#include "TestAVLTree.h"
#include <random>
#include <set>

void TestAVLTree()
{
	using namespace Thebe;

	AVLTree tree;

	std::random_device randomDevice;
	std::mt19937 generator(randomDevice());

	std::uniform_int_distribution<int> distribution(1, 100);
	std::set<int> numberSet;

	for (int i = 0; i < 100; i++)
	{
		int number = distribution(generator);
		tree.InsertNode(new Node(number));
		numberSet.insert(number);
	}

	THEBE_ASSERT(numberSet.size() == tree.GetNodeCount());
	THEBE_ASSERT(tree.IsAVLTree());

	std::vector<int> numberArray;
	for (int number : numberSet)
		numberArray.push_back(number);

	int i = 0;
	bool traversed = tree.Traverse([&i, &numberArray](const AVLTreeNode* node) -> bool
		{
			auto key = (const Key*)node->GetKey();
			return key->number == numberArray[i++];
		});

	THEBE_ASSERT(traversed);

	int j = tree.GetNodeCount() / 2;
	for (int i = 0; i < j; i++)
		tree.RemoveNode((AVLTreeNode*)tree.GetRootNode(), true);

	THEBE_ASSERT(tree.IsAVLTree());

	tree.Clear(true);
	THEBE_ASSERT(tree.GetNodeCount() == 0);
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