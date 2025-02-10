#pragma once

#include "ChineseCheckers/Factory.h"
#include "ChineseCheckers/Graph.h"
#include "ChineseCheckers/Marble.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Reference.h"

class Factory : public ChineseCheckers::Factory
{
public:
	Factory();
	virtual ~Factory();

	virtual ChineseCheckers::Graph* CreateGraph() override;
	virtual ChineseCheckers::Node* CreateNode(const ChineseCheckers::Vector& location, ChineseCheckers::Marble::Color color) override;
	virtual ChineseCheckers::Marble* CreateMarble(ChineseCheckers::Marble::Color color) override;
};

class Marble;

class Graph : public ChineseCheckers::Graph
{
public:
	Graph();
	virtual ~Graph();

	virtual void Clear() override;
	virtual bool MoveMarbleConditionally(const ChineseCheckers::MoveSequence& moveSequence) override;

public:
	std::list<Marble*> deadMarbleArray;
};

class Node : public ChineseCheckers::Node
{
public:
	Node(const ChineseCheckers::Vector& location, ChineseCheckers::Marble::Color color);
	virtual ~Node();

	Thebe::Vector3 GetLocation3D() const;
};

class Marble : public ChineseCheckers::Marble
{
public:
	Marble(Color color);
	virtual ~Marble();

public:
	int numLives;
	mutable Thebe::RefHandle collisionObjectHandle;
};