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
		virtual void IntegrateMotionUnconstrained(double timeStepSeconds) override;
		virtual Vector3 GetCenterOfMass() const override;
		virtual double GetTotalMass() const override;

	private:
		struct PointMass
		{
			Vector3* point;		///< This points into the collision object's vertex array.
			double mass;
		};

		struct Spring
		{
			PointMass* pointMassA;
			PointMass* pointMassB;
			double stiffness;
		};
		
		// TODO: Own array of point-masses.
		// TODO: Own array of springs.
	};
}