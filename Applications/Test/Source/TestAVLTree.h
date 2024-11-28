#pragma once

#include "Thebe/Containers/AVLTree.h"

class Key : public Thebe::AVLTreeKey
{
public:
	Key(int number);
	virtual ~Key();

	virtual bool IsLessThan(const AVLTreeKey* key) const override;
	virtual bool IsGreaterThan(const AVLTreeKey* key) const override;
	virtual bool IsEqualto(const AVLTreeKey* key) const override;
	virtual bool IsNotEqualto(const AVLTreeKey* key) const override;

	int number;
};

class Node : public Thebe::AVLTreeNode
{
public:
	Node(int number);
	virtual ~Node();

	virtual const Thebe::AVLTreeKey* GetKey() const override;
	virtual void SetKey(const Thebe::AVLTreeKey* givenKey) override;

	Key key;
};

void TestAVLTree();