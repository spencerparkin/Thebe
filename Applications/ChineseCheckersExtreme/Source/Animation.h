#pragma once

#include "Thebe/Reference.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Random.h"
#include "Thebe/EngineParts/RigidBody.h"
#include "ChineseCheckers/Graph.h"
#include "Chinesecheckers/MoveSequence.h"
#include <list>

class AnimationTask;

/**
 * 
 */
class AnimationProcessor
{
public:
	AnimationProcessor();
	virtual ~AnimationProcessor();

	void Animate(double deltaTimeSeconds, const ChineseCheckers::Graph* graph);
	void ClearQueue();
	void EnqueueTask(AnimationTask* animationTask);
	bool EnqueueAnimationForMoveSequence(const ChineseCheckers::MoveSequence& moveSequence, const ChineseCheckers::Graph* graph);
	void SnapAllMarblesToPosition(const ChineseCheckers::Graph* graph);

protected:
	Thebe::Random random;
	std::list<Thebe::Reference<AnimationTask>> animationTaskQueue;
};

/**
 * 
 */
class AnimationTask : public Thebe::ReferenceCounted
{
public:
	AnimationTask();
	virtual ~AnimationTask();

	/**
	 * An animation task is repeated asked to animated until
	 * this method returns false.
	 */
	virtual bool Animate(double deltaTimeSeconds) = 0;
};

/**
 * 
 */
class LaunchMarbleTask : public AnimationTask
{
public:
	LaunchMarbleTask();
	virtual ~LaunchMarbleTask();

	virtual bool Animate(double deltaTimeSeconds) override;

	Thebe::Reference<Thebe::RigidBody> rigidBody;
	Thebe::Vector3 landingTarget;
	double totalFlightTime;
};

/**
 * 
 */
class WaitForMarbleImpactTask : public AnimationTask
{
public:
	WaitForMarbleImpactTask();
	virtual ~WaitForMarbleImpactTask();

	virtual bool Animate(double deltaTimeSeconds) override;

	Thebe::Reference<Thebe::CollisionObject> collisionObject;
};

/**
 * 
 */
class DelayTask : public AnimationTask
{
public:
	DelayTask();
	virtual ~DelayTask();

	virtual bool Animate(double deltaTimeSeconds) override;

	double delayTimeRemainingSeconds;
};

/**
 * 
 */
class StabilizeMarbleOnPlatformTask : public AnimationTask
{
public:
	StabilizeMarbleOnPlatformTask();
	virtual ~StabilizeMarbleOnPlatformTask();

	virtual bool Animate(double deltaTimeSeconds) override;

	Thebe::Reference<Thebe::RigidBody> rigidBody;
	Thebe::Vector3 targetLocation;
	double timeLimitRemainingSeconds;
};

/**
 * 
 */
class DramaticDeathTask : public AnimationTask
{
public:
	DramaticDeathTask();
	virtual ~DramaticDeathTask();

	virtual bool Animate(double deltaTimeSeconds) override;

	Thebe::Random* random;
	const ChineseCheckers::Graph* graph;
	Thebe::Reference<Thebe::RigidBody> rigidBody;
	double totalFlightTime;
};

class StartFollowCamTask : public AnimationTask
{
public:
	StartFollowCamTask();
	virtual ~StartFollowCamTask();

	virtual bool Animate(double deltaTimeSeconds) override;

	Thebe::Reference<Thebe::CollisionObject> collisionObject;
};

class StopFollowCamTask : public AnimationTask
{
public:
	StopFollowCamTask();
	virtual ~StopFollowCamTask();

	virtual bool Animate(double deltaTimeSeconds) override;
};