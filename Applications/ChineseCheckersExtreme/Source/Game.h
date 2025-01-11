#pragma once

#include "Thebe/Reference.h"
#include "Thebe/EngineParts/Space.h"
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

		int playerID;
		double health;
		double attackPower;
	};

	class Node : public Thebe::ReferenceCounted
	{
	public:
		Node();
		virtual ~Node();

		void RemoveNullAdjacencies();

		std::vector<Node*> adjacentNodeArray;
		Thebe::Vector3 location;
		Occupant* occupant;
		int zoneID;
	};

	virtual int GetMaxPossiblePlayers() const = 0;
	virtual const char* GetGameType() const = 0;

	static ChineseCheckersGame* Factory(const char* gameType);

	/**
	 * Generate the graph data-structure on which the game is played.
	 */
	virtual void GenerateGraph() = 0;

	/**
	 * Populate the scene with a visual manifestation of the game.
	 * Of course, the human-controlled clients will do this, but the
	 * server will not.  Computer-controlled clients won't need to
	 * do this either.
	 */
	virtual bool PopulateScene(Thebe::Space* space) = 0;

	virtual bool GetInitialZoneForPlayer(int playerID, int& initialZoneID) = 0;
	virtual bool GetTargetZoneForPlayer(int playerID, int& targetZoneID) = 0;

	void Clear();

protected:

	std::vector<Thebe::Reference<Node>> nodeArray;
	std::vector<Thebe::Reference<Occupant>> occupantArray;
};