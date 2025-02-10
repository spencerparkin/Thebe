#include "Animation.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/PhysicsSystem.h"
#include "Thebe/CollisionSystem.h"
#include "Factory.h"

using namespace Thebe;

//---------------------------------- AnimationProcessor ----------------------------------

AnimationProcessor::AnimationProcessor()
{
}

/*virtual*/ AnimationProcessor::~AnimationProcessor()
{
}

void AnimationProcessor::Animate(double deltaTimeSeconds, const ChineseCheckers::Graph* graph)
{
	if (this->animationTaskQueue.size() == 0)
		return;

	AnimationTask* animationTask = this->animationTaskQueue.front().Get();
	if (!animationTask->Animate(deltaTimeSeconds))
	{
		this->animationTaskQueue.pop_front();

		if (this->animationTaskQueue.size() == 0)
			this->SnapAllMarblesToPosition(graph);
	}
}

void AnimationProcessor::ClearQueue()
{
	this->animationTaskQueue.clear();
}

void AnimationProcessor::EnqueueTask(AnimationTask* animationTask)
{
	this->animationTaskQueue.push_back(animationTask);
}

bool AnimationProcessor::EnqueueAnimationForMoveSequence(const ChineseCheckers::MoveSequence& moveSequence, const ChineseCheckers::Graph* graph)
{
	if (moveSequence.nodeIndexArray.size() == 0)
		return false;

	auto marble = dynamic_cast<Marble*>(graph->GetNodeArray()[moveSequence.nodeIndexArray[0]]->GetOccupant());
	if (!marble)
		return false;

	Reference<CollisionObject> collisionObject;
	if (!HandleManager::Get()->GetObjectFromHandle(marble->collisionObjectHandle, collisionObject))
		return false;

	Reference<RigidBody> rigidBody;
	RefHandle handle = (RefHandle)collisionObject->GetPhysicsData();
	HandleManager::Get()->GetObjectFromHandle(handle, rigidBody);

	std::vector<Marble*> doomedMarbleArray;

	for (int i = 0; i < (int)moveSequence.nodeIndexArray.size() - 1; i++)
	{
		auto nodeSource = (Node*)graph->GetNodeArray()[moveSequence.nodeIndexArray[i]];
		auto nodeTarget = (Node*)graph->GetNodeArray()[moveSequence.nodeIndexArray[i + 1]];
		auto nodeJumped = (Node*)Node::FindMutualAdjacency(nodeSource, nodeTarget);

		auto launchTask = new LaunchMarbleTask();
		launchTask->totalFlightTime = 2.5;
		launchTask->rigidBody = rigidBody;
		this->EnqueueTask(launchTask);

		// In the extreme version of Chinese Checkers, we don't jump over opponent marbles; we jump onto them!
		if (!nodeJumped)
			launchTask->landingTarget = nodeTarget->GetLocation3D();
		else
		{
			auto marbleJumped = (Marble*)nodeJumped->GetOccupant();
			if(marbleJumped->GetColor() == marble->GetColor())
				launchTask->landingTarget = nodeTarget->GetLocation3D();
			else
			{
				launchTask->landingTarget = nodeJumped->GetLocation3D();
				if (marbleJumped->numLives == 1)
					doomedMarbleArray.push_back(marbleJumped);
			}
		}

		auto waitForImpactTask = new WaitForMarbleImpactTask();
		waitForImpactTask->collisionObject = collisionObject;
		this->EnqueueTask(waitForImpactTask);

		auto delayTask = new DelayTask();
		delayTask->delayTimeRemainingSeconds = 1.0;
		this->EnqueueTask(delayTask);
	}

	auto node = (Node*)graph->GetNodeArray()[moveSequence.nodeIndexArray[moveSequence.nodeIndexArray.size() - 1]];
	auto stabilizeTask = new StabilizeMarbleOnPlatformTask();
	stabilizeTask->targetLocation = node->GetLocation3D() + Vector3(0.0, 2.5, 0.0);
	stabilizeTask->rigidBody = rigidBody;
	this->EnqueueTask(stabilizeTask);

	for (Marble* doomedMarble : doomedMarbleArray)
	{
		if (!HandleManager::Get()->GetObjectFromHandle(doomedMarble->collisionObjectHandle, collisionObject))
			continue;

		handle = (RefHandle)collisionObject->GetPhysicsData();
		if (!HandleManager::Get()->GetObjectFromHandle(handle, rigidBody))
			continue;

		auto deathTask = new DramaticDeathTask();
		deathTask->rigidBody = rigidBody;
		deathTask->totalFlightTime = 5.0;
		deathTask->graph = graph;
		deathTask->random = &this->random;
		this->EnqueueTask(deathTask);

		auto delayTask = new DelayTask();
		delayTask->delayTimeRemainingSeconds = 1.0;
		this->EnqueueTask(delayTask);
	}

	return true;
}

void AnimationProcessor::SnapAllMarblesToPosition(const ChineseCheckers::Graph* graph)
{
	const std::vector<ChineseCheckers::Node*>& nodeArray = graph->GetNodeArray();
	for (const auto& nativeNode : nodeArray)
	{
		const ChineseCheckers::Marble* nativeMarble = nativeNode->GetOccupant();
		if (!nativeMarble)
			continue;

		auto marble = dynamic_cast<const Marble*>(nativeMarble);
		if (!marble)
			continue;

		auto node = dynamic_cast<const Node*>(nativeNode);
		if (!node)
			continue;

		Reference<CollisionObject> collisionObject;
		if (!HandleManager::Get()->GetObjectFromHandle(marble->collisionObjectHandle, collisionObject))
			continue;

		Transform objectToWorld;
		objectToWorld.SetIdentity();
		objectToWorld.translation = node->GetLocation3D() + Vector3(0.0, 2.5, 0.0);

		collisionObject->SetObjectToWorld(objectToWorld);

		Reference<PhysicsObject> physicsObject;
		RefHandle handle = (RefHandle)collisionObject->GetPhysicsData();
		if (!HandleManager::Get()->GetObjectFromHandle(handle, physicsObject))
			continue;

		physicsObject->ZeroMomentum();
	}
}

//---------------------------------- AnimationTask ----------------------------------

AnimationTask::AnimationTask()
{
}

/*virtual*/ AnimationTask::~AnimationTask()
{
}

//---------------------------------- LaunchMarbleTask ----------------------------------

LaunchMarbleTask::LaunchMarbleTask()
{
	this->totalFlightTime = 0.0;
}

/*virtual*/ LaunchMarbleTask::~LaunchMarbleTask()
{
}

/*virtual*/ bool LaunchMarbleTask::Animate(double deltaTimeSeconds)
{
	const Transform& objectToWorld = this->rigidBody->GetObjectToWorld();

	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	this->rigidBody->GetGraphicsEngine(graphicsEngine);
	const Vector3& accelerationDueToGravity = graphicsEngine->GetPhysicsSystem()->GetGravity();

	Vector3 delta = this->landingTarget - objectToWorld.translation;
	Vector3 initialVelocity = delta / this->totalFlightTime - accelerationDueToGravity * this->totalFlightTime / 2.0;
	double totalMass = this->rigidBody->GetTotalMass();

	this->rigidBody->SetLinearMomentum(initialVelocity * totalMass);

	double spinTorque = 5000.0;
	Vector3 torque = Vector3::YAxis().Cross(delta).Normalized() * spinTorque;
	this->rigidBody->AddTransientTorque(torque);

	return false;
}

//---------------------------------- WaitForMarbleImpactTask ----------------------------------

WaitForMarbleImpactTask::WaitForMarbleImpactTask()
{
}

/*virtual*/ WaitForMarbleImpactTask::~WaitForMarbleImpactTask()
{
}

/*virtual*/ bool WaitForMarbleImpactTask::Animate(double deltaTimeSeconds)
{
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	this->collisionObject->GetGraphicsEngine(graphicsEngine);

	std::vector<Reference<CollisionSystem::Collision>> collisionArray;
	graphicsEngine->GetCollisionSystem()->FindAllCollisions(this->collisionObject, collisionArray);
	if (collisionArray.size() > 0)
		return false;

	return true;
}

//---------------------------------- DelayTask ----------------------------------

DelayTask::DelayTask()
{
	this->delayTimeRemainingSeconds = 0.0;
}

/*virtual*/ DelayTask::~DelayTask()
{
}

/*virtual*/ bool DelayTask::Animate(double deltaTimeSeconds)
{
	this->delayTimeRemainingSeconds -= deltaTimeSeconds;
	if (this->delayTimeRemainingSeconds <= 0.0)
		return false;

	return true;
}

//---------------------------------- StabilizeMarbleOnPlatformTask ----------------------------------

StabilizeMarbleOnPlatformTask::StabilizeMarbleOnPlatformTask()
{
	this->timeLimitRemainingSeconds = 3.0;
}

/*virtual*/ StabilizeMarbleOnPlatformTask::~StabilizeMarbleOnPlatformTask()
{
}

/*virtual*/ bool StabilizeMarbleOnPlatformTask::Animate(double deltaTimeSeconds)
{
	this->timeLimitRemainingSeconds -= deltaTimeSeconds;

	Transform objectToWorld = this->rigidBody->GetObjectToWorld();
	Vector3 delta = this->targetLocation - objectToWorld.translation;
	double length = delta.Length();
	static double threshold = 1.0;
	if (length < threshold || this->timeLimitRemainingSeconds <= 0.0)
	{
		this->rigidBody->ZeroMomentum();
		objectToWorld.translation = this->targetLocation;
		this->rigidBody->SetObjectToWorld(objectToWorld);
		return false;
	}

	static double forceFactor = 50.0;
	Vector3 force = delta * forceFactor;
	this->rigidBody->AddTransientForce(force);
	return true;
}

//---------------------------------- DramaticDeathTask ----------------------------------

DramaticDeathTask::DramaticDeathTask()
{
	this->totalFlightTime = 5.0;
	this->graph = nullptr;
	this->random = nullptr;
}

/*virtual*/ DramaticDeathTask::~DramaticDeathTask()
{
}

/*virtual*/ bool DramaticDeathTask::Animate(double deltaTimeSeconds)
{
	const Transform& objectToWorld = this->rigidBody->GetObjectToWorld();

	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	this->rigidBody->GetGraphicsEngine(graphicsEngine);
	const Vector3& accelerationDueToGravity = graphicsEngine->GetPhysicsSystem()->GetGravity();

	AxisAlignedBoundingBox box;
	box.MakeReadyForExpansion();
	for (const auto& nativeNode : this->graph->GetNodeArray())
	{
		auto node = (Node*)nativeNode;
		box.Expand(node->GetLocation3D());
	}

	Vector3 center = box.GetCenter();
	double randomAngle = this->random->InRange(0.0, 2.0 * THEBE_PI);
	Vector3 randomDirection(::cos(randomAngle), 0.0, -::sin(randomAngle));

	double width, height, depth;
	box.GetDimensions(width, height, depth);
	double throwDistance = THEBE_MAX(THEBE_MAX(width, height), depth) * 1.1;

	Vector3 landingTarget = center + randomDirection * throwDistance;

	Vector3 delta = landingTarget - objectToWorld.translation;
	Vector3 initialVelocity = delta / this->totalFlightTime - accelerationDueToGravity * this->totalFlightTime / 2.0;
	double totalMass = this->rigidBody->GetTotalMass();

	this->rigidBody->SetLinearMomentum(initialVelocity * totalMass);

	double spinTorque = 5000.0;
	Vector3 torque = Vector3::YAxis().Cross(delta).Normalized() * spinTorque;
	this->rigidBody->AddTransientTorque(torque);

	return false;
}