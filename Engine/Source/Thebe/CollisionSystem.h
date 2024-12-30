#pragma once

#include "Thebe/Common.h"
#include "Thebe/BoundingVolumeHierarchy.h"
#include <map>

namespace Thebe
{
	class CollisionObject;
	class DynamicLineRenderer;

	/**
	 * This is a basic system for keeping track of various shapes in
	 * space, making it easy to determine which pairs of shapes overlap
	 * one another, and *how* they overlap.  Note that it is completely
	 * decoupled from the physics system.  The physics system is a user
	 * of the collision system, but not vice-versa.  Put another way, the
	 * physics system knows about the collision system, but the collision
	 * system knows nothing about physics.  As such, you can use the
	 * collision system on its own without doing any sort of physics,
	 * if you want.  You could also say that the collision system is a
	 * lower-level sub-system than the physics system.
	 */
	class THEBE_API CollisionSystem
	{
	public:
		CollisionSystem();
		virtual ~CollisionSystem();

		bool TrackObject(CollisionObject* collisionObject);
		bool UntrackObject(CollisionObject* collisionObject);
		void UntrackAllObjects();
		void SetWorldBox(const AxisAlignedBoundingBox& worldBox);
		const AxisAlignedBoundingBox& GetWorldBox() const;

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