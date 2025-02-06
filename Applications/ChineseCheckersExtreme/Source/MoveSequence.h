#pragma once

#include "JsonValue.h"
#include "ChineseCheckers/Graph.h"

/**
 * Note that for this class to work in a network setting, the order
 * of the nodes in a graph must be deterministic across different
 * instances of the graph.
 */
class MoveSequence	// TODO: Move this into core library?  Can't see why not.
{
public:
	MoveSequence();
	virtual ~MoveSequence();

	bool ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const;
	bool FromJson(const ParseParty::JsonValue* jsonValue);
	bool Extend(int nodeIndex, ChineseCheckers::Graph* graph, ChineseCheckers::Marble::Color color);
	bool ToMove(ChineseCheckers::Graph::Move& move, ChineseCheckers::Graph* graph) const;
	bool FromMove(const ChineseCheckers::Graph::Move& move, ChineseCheckers::Graph* graph);

public:
	std::vector<int> nodeIndexArray;
};