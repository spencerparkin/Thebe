#pragma once

#include "Thebe/EngineParts/PhysicsObject.h"

namespace Thebe
{
	/**
	 * These are spring lattice systems -- a bunch of point masses
	 * held together by springs.  Taken together, these can approximate
	 * rigid bodies as the springs get stiffer, but as they do, the simulation
	 * becomes more and more unstable.
	 */
	class THEBE_API FloppyBody : public PhysicsObject
	{
	public:
		FloppyBody();
		virtual ~FloppyBody();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual void AccumulateForcesAndTorques(PhysicsSystem* physicsSystem) override;
		virtual void IntegrateMotionUnconstrained(double timeStepSeconds) override;
		virtual Vector3 GetCenterOfMass() const override;
		virtual double GetTotalMass() const override;
		virtual void DebugDraw(DynamicLineRenderer* lineRenderer) const override;
		virtual void SetObjectToWorld(const Transform& objectToWorld) override;
		virtual Transform GetObjectToWorld() const override;

	private:
		void SetSpringEquilibriumLengths();

		struct PointMass
		{
			unsigned int offset;		//< This is an offset into the shape's array of vertices.
			double mass;
			Vector3 totalForce;
			Vector3 velocity;
		};

		struct Spring
		{
			unsigned int offset[2];
			double stiffness;
			double equilibriumLength;
		};
		
		bool GetSpringWorldVertices(const Spring& spring, Vector3& vertexA, Vector3& vertexB);
		bool GetWorldVertex(const PointMass& pointMass, Vector3& vertex);

		std::vector<PointMass> pointMassArray;
		std::vector<Spring> springArray;
	};
}