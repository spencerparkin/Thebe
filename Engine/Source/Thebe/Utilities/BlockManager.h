#pragma once

#include "Thebe/Common.h"
#include "Thebe/Containers/LinkedList.h"
#include "Thebe/Containers/AVLTree.h"

namespace Thebe
{
	/**
	 * Manage a block of memory.  Note that we don't need a pointer to
	 * the block of memory we manage.  The user of the class can hang
	 * onto that.  All we need to know here is the size of the block to
	 * be managed.
	 */
	class THEBE_API BlockManager
	{
	public:
		BlockManager();
		virtual ~BlockManager();

		class BlockNode;

		/**
		 * Reset the manager to a heap where the full range of memory
		 * from 0 to the given size is available for allocation.
		 */
		void Reset(uint64_t size);

		/**
		 * Find and return a block of the given size at an offset
		 * that is a multiple of the given alignment.  False is
		 * returned here if no such block can be found.  Note that
		 * if enough memory is available, then the problem could
		 * be fragmentation of the heap.  Also note that we don't
		 * try to find a block of best fit here.  We just use the
		 * first free block that is big enough.
		 */
		BlockNode* Allocate(uint64_t size, uint64_t align);

		/**
		 * Return the given block of memory to the heap.  Failure
		 * can occur here if the given block wasn't taken from
		 * this block manager.
		 */
		bool Deallocate(BlockNode* blockNode);

		/**
		 * Run through the internal data-structure and make sure it's valid.
		 */
		bool ConsistencyCheckPasses() const;

		/**
		 * 
		 */
		class THEBE_API Block : public AVLTreeKey, public LinkedListNode
		{
			friend class BlockNode;
			friend class BlockManager;

		public:
			Block(BlockManager* blockManager);
			virtual ~Block();

			virtual bool IsLessThan(const AVLTreeKey* key) const override;
			virtual bool IsGreaterThan(const AVLTreeKey* key) const override;
			virtual bool IsEqualto(const AVLTreeKey* key) const override;
			virtual bool IsNotEqualto(const AVLTreeKey* key) const override;

			enum State
			{
				ALLOCATED,
				FREE
			};

			uint64_t GetOffset() const;
			uint64_t GetSize() const;

		private:
			BlockManager* blockManager;
			uint64_t offset;
			uint64_t size;
			State state;
		};

		/**
		 * These are the units of allocation that can be taken out and
		 * put back into the heap.
		 */
		class THEBE_API BlockNode : public AVLTreeNode
		{
			friend class BlockManager;

		public:
			BlockNode(Block* block);
			virtual ~BlockNode();

			virtual const AVLTreeKey* GetKey() const override;
			virtual void SetKey(const AVLTreeKey* givenKey) override;

			Block* GetBlock();

		private:
			Block* block;
		};

	private:
		bool TryAllocate(BlockNode* blockNode, uint64_t size, uint64_t align);

		uint64_t freeMemAvailable;

		// This container will maintain the physical order of the blocks in memory.
		LinkedList blockList;

		// This container will index the free blocks in memory by size for quicker allocation.
		AVLTree freeBlockTree;
	};
}