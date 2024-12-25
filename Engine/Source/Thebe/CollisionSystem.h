#pragma once

#include "Thebe/Common.h"
#include "Thebe/BoundingVolumeHierarchy.h"
#include <map>

namespace Thebe
{
	class CollisionObject;
	class DynamicLineRenderer;

	/**
	 * 
	 */
	// TODO: Derive a class from this one called PhysicsSystem?  Rigid-body dynamics are a tall order, but would be great.
	class THEBE_API CollisionSystem
	{
	public:
		CollisionSystem();
		virtual ~CollisionSystem();

		bool TrackObject(CollisionObject* collisionObject);
		bool UntrackObject(CollisionObject* collisionObject);
		void UntrackAllObjects();

		/**
		 * The main feature of the collision system is to produce instances
		 * of this class upon which the application can act to move, separate,
		 * or otherwise acknowledge overlaps of, collision objects.
		 */
		class Collision : public ReferenceCounted
		{
			friend class CollisionSystem;

		public:
			Collision();
			virtual ~Collision();

			bool StillValid() const;

			/**
			 * These are two objects in collision, meaning that they
			 * share at least one point in common.
			 */
			Reference<CollisionObject> objectA, objectB;

			/**
			 * When added to the position of objectA, or subracted from
			 * the position of objectB, the objects will then share at
			 * most one point in common.
			 */
			Vector3 separationDelta;

			/**
			 * These are the principle points of contact between the two the two objects.
			 * Of cousre, two objects can be touching in an area of infinitely many points,
			 * in which case, the convex hull of this point array should outline that area.
			 */
			std::vector<Vector3> contactPointArray;

		private:
			/**
			 * 
			 */
			uint64_t validFrameA;
			uint64_t validFrameB;
		};

		/**
		 * As quickly as possible, find all collisions with which the given
		 * collision object is involved.
		 */
		void FindAllCollisions(CollisionObject* collisionObject, std::vector<Reference<Collision>>& collisionArray);

		void DebugDraw(DynamicLineRenderer* lineRenderer, uint32_t& lineOffset) const;

	protected:
		std::string MakeCollisionCacheKey(const CollisionObject* objectA, const CollisionObject* objectB);

		Reference<BVHTree> boxTree;
		std::map<RefHandle, Reference<CollisionObject>> collisionObjectMap;
		std::unordered_map<std::string, Reference<Collision>> collisionCacheMap;
	};
}