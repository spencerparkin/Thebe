#pragma once

#include "JsonValue.h"
#include "ChineseCheckers/Graph.h"

namespace ChineseCheckers
{
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
		
		bool ToMove(Graph::Move& move, Graph* graph) const;
		bool FromMove(const Graph::Move& move, Graph* graph);

		bool Extend(int nodeIndex, Graph* graph, Marble::Color color);

	public:
		std::vector<int> nodeIndexArray;
	};
}