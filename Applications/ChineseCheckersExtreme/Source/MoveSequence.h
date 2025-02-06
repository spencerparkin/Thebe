#pragma once

#include "JsonValue.h"
#include "ChineseCheckers/Graph.h"

/**
 * Note that for this class to work in a network setting, the order
 * of the nodes in a graph must be deterministic across different
 * instances of the graph.
 */
class MoveSequence
{
public:
	MoveSequence();
	virtual ~MoveSequence();

	bool ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const;
	bool FromJson(const ParseParty::JsonValue* jsonValue);
	bool Extend(int nodeIndex, ChineseCheckers::Graph* graph, ChineseCheckers::Marble::Color color);
	bool MakeMove(ChineseCheckers::Graph::Move& move, ChineseCheckers::Graph* graph) const;

public:
	std::vector<int> nodeIndexArray;
};