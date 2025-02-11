#pragma once

#include "Marble.h"
#include "Vector.h"
#include "JsonValue.h"
#include <vector>
#include <functional>
#include <map>

namespace ChineseCheckers
{
	class Node;
	class Factory;
	class MoveSequence;

	/**
	 * An instance of this class is the game board.
	 */
	class Graph
	{
	public:
		Graph();
		virtual ~Graph();

		struct Move
		{
			Node* sourceNode;
			Node* targetNode;

			double CalcDistanceInDirection(const Vector& unitLengthDirection) const;
		};

		struct BestMovesCollection
		{
			BestMovesCollection();

			double longestDistance;
			std::vector<Move> moveArray;

			void AddMove(const Move& move, const Vector& unitLengthDirection);
			bool GetRandom(Move& move) const;
		};

		virtual void Clear();
		void AddNode(Node* node);

		Graph* Clone(Factory* factory) const;

		virtual bool ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const;
		virtual bool FromJson(const ParseParty::JsonValue* jsonValue, Factory* factory);

		bool SetColorTarget(Marble::Color sourceColor, Marble::Color targetColor);

		const std::vector<Node*>& GetNodeArray() const;

		/**
		 * According to the rules of Chinese Checkers, tell us if a marble
		 * can hop to all the given nodes in a single turn.
		 */
		bool IsValidMoveSequence(const MoveSequence& moveSequence) const;

		/**
		 * Tell us if all marbles of the given color have reached their target nodes.
		 * Note that this only considers marbles that are presently on the board.
		 * (A varient of the game might take marbles off the board temporarily.)
		 */
		bool AllMarblesAtTarget(Marble::Color marbleColor) const;

		/**
		 * Take the occupant from the given source and place it
		 * at the given destination.  Note that we do not do anything
		 * here to enforce the rules of the game, and so we're
		 * unconditional here in that sense.  We can fail here if the
		 * move can't physically take place.
		 */
		bool MoveMarbleUnconditionally(const Move& move);

		/**
		 * This is like @ref MoveMarbleUnconditionally, but here we make
		 * sure that the rules of the game are enforced.
		 */
		virtual bool MoveMarbleConditionally(const MoveSequence& moveSequence);

		/**
		 * According to the rules of the game of Chinese Checkers, find a
		 * valid set of moves for the given marble color that moves each marble
		 * as far as possible in the given direction.  Note that this does
		 * not take any planning into account.  It is purely a greedy tactic.
		 * 
		 * I don't think that this stratagy would entirely work for all graph topologies.
		 */
		bool FindBestMoves(Marble::Color marbleColor, BestMovesCollection& bestMovesCollection, const Vector& generalDirection) const;

		/**
		 * Calculate and return the general direction that marbles of the given color are trying to go.
		 */
		bool CalcGeneralDirection(Marble::Color color, Vector& generalDirection) const;

		/**
		 * Calculate and return the center of the zone with the given color.
		 */
		Vector CalcZoneCentroid(Marble::Color color) const;

		/**
		 * Calculate and return the center of the marbles with the given color.
		 */
		Vector CalcMarbleCentroid(Marble::Color color) const;

	protected:
		void WipeParentPointers() const;
		void AssignOffsets() const;

		std::vector<Node*> nodeArray;
		std::map<Marble::Color, Marble::Color> colorMap;
	};

	/**
	 * Instances of this class are the elements of the game board,
	 * or the places where marbles can land as they hop along.
	 * Varients of the game of Chinese Checkers might inherit from
	 * this class to add additional characteristics.
	 */
	class Node
	{
	public:
		Node(const Vector& location, Marble::Color color);
		virtual ~Node();

		virtual bool ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const;
		virtual bool FromJson(const ParseParty::JsonValue* jsonValue, const std::vector<Node*>& nodeArray, Factory* factory);

		void SetLocation(const Vector& location);
		const Vector& GetLocation() const;
		void SetColor(Marble::Color color);
		Marble::Color GetColor() const;
		std::shared_ptr<Marble> GetOccupant();
		const std::shared_ptr<Marble> GetOccupant() const;
		void SetOccupant(std::shared_ptr<Marble> marble);
		int GetNumAdjacentNodes() const;
		void AddAjacentNode(Node* node);
		Node* GetAdjacentNode(int i) const;
		Node* GetAdjacentNode(const Vector& givenDirection) const;
		Node* JumpInDirection(int i) const;
		bool IsAdjacentTo(const Node* node) const;
		bool ForAllJumpsDFS(std::vector<const Node*>& nodeStack, std::function<bool(Node*)> callback) const;
		bool ForAllJumpsBFS(std::function<bool(Node*)> callback) const;
		void RemoveNullAdjacencies();

		static const Node* FindMutualAdjacency(const Node* nodeA, const Node* nodeB);

		Node* GetParent();
		const Node* GetParent() const;
		void SetParent(Node* parent) const;

		int GetOffset() const;
		void SetOffset(int offset) const;

	protected:
		std::shared_ptr<Marble> occupant;
		Vector location;
		Marble::Color color;
		std::vector<Node*> adjacentNodeArray;
		mutable int offset;
		mutable Node* parent;
	};
}