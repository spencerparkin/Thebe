#pragma once

#include "Thebe/Reference.h"
#include "Thebe/Math/Vector3.h"
#include "JsonValue.h"

/**
 * An instance of this class represents the entire game state.
 * This state is replicated and synchronized across the server and clients.
 * 
 * For the most part, this is just the traditional game of Chinese Checkers,
 * but with the following additions.  First, each marble or piece, if you will,
 * has some amount of health and attack power.  Jumping over one of your own
 * pieces increases your attack power, and jumping over an opponent's piece
 * deals damage to their health.  If a marble's health goes to zero, it is
 * flung off the board and can only rejoin the game (with full health) after
 * completing a move with two or more hops.  (Perhaps this conditions needs
 * more thought.)  In any case, the rest of the rules apply as normal.  You
 * must get all of your marbles into the target zone before any of your
 * opponents do the same.
 * 
 * To make the game more entertaining, perhaps, the board pieces will be
 * rigid body boxes that hop around.  Instead of jumping over an opponent
 * piece, they will crash into those pieces (dealing damage).  Forces will
 * be exerted on all pieces to keep them approximately where they're supposed
 * to lie on the board.  This will given the entire board a more organic feel.
 * 
 * Note that in this class we enforce all the rules of the game except for
 * the concept of who's turn it is.  Making sure turns are taken in sequence
 * is up to the user of the class.
 */
class ChineseCheckersGame : public Thebe::ReferenceCounted
{
public:
	ChineseCheckersGame();
	virtual ~ChineseCheckersGame();

	bool ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue) const;
	bool FromJson(const ParseParty::JsonValue* jsonRootValue);

	class Occupant : public Thebe::ReferenceCounted
	{
	public:
		Occupant();
		virtual ~Occupant();

		Thebe::RefHandle collisionObjectHandle;
		int sourceZoneID;
		int targetZoneID;
		double health;
		double attackPower;
	};

	class Node : public Thebe::ReferenceCounted
	{
	public:
		Node();
		virtual ~Node();

		void RemoveNullAdjacencies();

		Node* GetAdjacencyAndDirection(int i, Thebe::Vector3& unitDirection);
		Node* GetAdjacencyInDirection(const Thebe::Vector3& unitDirection);

		bool IsAdjacentTo(Node* node);

		bool FindWithHops(Node* targetNode, std::vector<Node*>& nodePathArray);

		std::vector<Node*> adjacentNodeArray;
		Thebe::Vector3 location;
		Occupant* occupant;
		int zoneID;
		Node* parentNode;
	};

	virtual int GetMaxPossiblePlayers() const = 0;
	virtual const char* GetGameType() const = 0;

	static ChineseCheckersGame* Factory(const char* gameType);

	/**
	 * Generate the graph data-structure on which the game is played.
	 * Also generate occupants for the number of players given.
	 */
	virtual bool GenerateGraph(int numPlayers) = 0;

	/**
	 * Return what color is used to represent the given zone.
	 */
	virtual bool GetZoneColor(int zoneID, Thebe::Vector3& color) = 0;

	/**
	 * Determine how players should be allocated to zone/occupants as they join.
	 */
	virtual void GenerateFreeZoneIDStack(std::vector<int>& freeZoneIDStack) = 0;

	int GetNextZone(int zoneID);
	bool IsZoneBeingUsed(int zoneID);
	int GetNumActivePlayers();

	void Clear();

	const std::vector<Thebe::Reference<Node>>& GetNodeArray() const;

	bool FindLegalPath(Node* sourceNode, Node* targetNode, std::vector<Node*>& nodePathArray);
	bool IsPathLegal(const std::vector<Node*>& nodePathArray, std::vector<Node*>* hoppedNodesArray = nullptr);
	bool ExecutePath(const std::vector<Node*>& nodePathArray);

	int NodeToOffset(Node* node);
	Node* NodeFromOffset(int offset);

	bool NodeArrayToOffsetArray(const std::vector<Node*>& nodePathArray, std::vector<int>& nodeOffsetArray);
	bool NodeArrayFromOffsetArray(std::vector<Node*>& nodePathArray, const std::vector<int>& nodeOffsetArray);

protected:

	std::vector<Thebe::Reference<Node>> nodeArray;
	std::vector<Thebe::Reference<Occupant>> occupantArray;
};