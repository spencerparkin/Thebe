#pragma once

#include "Thebe/Math/AxisAlignedBoundingBox.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/Reference.h"

namespace Thebe
{
	class BVHNode;
	class BVHObject;

	/**
	 * This is a bounding volume hierarchy or bounding box tree.
	 * Its primary use is to spatially partition a bunch of objects
	 * in space to accelerate the task of termining what objects, if
	 * any, overlap a given bounding box.
	 */
	class THEBE_API BVHTree : public ReferenceCounted
	{
	public:
		BVHTree();
		virtual ~BVHTree();

		void SetWorldBox(const AxisAlignedBoundingBox& worldBox);
		const AxisAlignedBoundingBox& GetWorldBox() const;

		bool AddObject(BVHObject* object);
		bool RemoveObject(BVHObject* object);
		void RemoveAllObjects();
		void FindObjects(const AxisAlignedBoundingBox& worldBox, std::list<BVHObject*>& objectList);

	private:
		Reference<BVHNode> rootNode;
		AxisAlignedBoundingBox worldBox;
	};

	/**
	 * These are the nodes of the BVH.
	 */
	class THEBE_API BVHNode : public ReferenceCounted
	{
		friend class BVHTree;
		friend class BVHObject;

	public:
		BVHNode();
		virtual ~BVHNode();

		RefHandle GetTreeHandle() const;
		const AxisAlignedBoundingBox& GetWorldBox() const;

		void RemoveObject(BVHObject* object);
		void AddObject(BVHObject* object);

	private:
		std::list<Reference<BVHObject>> objectList;
		BVHNode* parentNode;
		std::vector<Reference<BVHNode>> childNodeArray;
		RefHandle treeHandle;
		AxisAlignedBoundingBox worldBox;
	};

	/**
	 * These are objects spatially organized by the BVH.
	 */
	class BVHObject : public ReferenceCounted
	{
		friend class BVHTree;

	public:
		BVHObject();
		virtual ~BVHObject();

		/**
		 * It is not necessary to override this method.  If you do, return a new object
		 * representing the part of this object that fits in the given box.  Note that
		 * it is imparrative that you return false if the given box has a volume below
		 * a certain threshold.  Otherwise, recursion will blow the call-stack.  Choose a
		 * threshold based on how small the leaf nodes of the BVH tree should typically be.
		 */
		virtual bool Cut(const AxisAlignedBoundingBox& box, Reference<BVHObject>& newObject);

		/**
		 * Return the smallest possible bounding box for this object
		 * in world space.
		 */
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const = 0;

		/**
		 * This should get called after any change that would cause
		 * the @ref GetWorldBoundingBox method to return something
		 * different than what it did when last this object's location
		 * within the BVH was determined.  In other words, if the object
		 * moves through space, this should be called regularly before
		 * trying to use the BVH to do queries.
		 * 
		 * If this object is static, cutting it into peices so that it
		 * can be pushed deeper into the tree may improve query performance.
		 */
		bool UpdateBVHLocation(bool allowCuts = false);

	private:
		bool GetNode(Reference<BVHNode>& node);
		void SetNode(BVHNode* Node);

		RefHandle nodeHandle;
	};
}