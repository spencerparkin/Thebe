#include "BlockManager.h"

using namespace Thebe;

//------------------------------------- BlockManager -------------------------------------

BlockManager::BlockManager()
{
	this->freeMemAvailable = 0L;
}

/*virtual*/ BlockManager::~BlockManager()
{
	this->Reset(0);
}

void BlockManager::Reset(uint64_t size)
{
	this->freeBlockTree.Clear(true);	// Must clear tree before keys go stale.
	this->blockList.Clear(true);

	this->freeMemAvailable = size;

	if (size > 0)
	{
		auto block = new Block(this);
		block->offset = 0;
		block->size = size;
		this->blockList.InsertNodeAfter(block);
		auto blockNode = new BlockNode(block);
		this->freeBlockTree.InsertNode(blockNode);
	}
}

BlockManager::BlockNode* BlockManager::Allocate(uint64_t size, uint64_t align)
{
	if (this->freeMemAvailable == 0 || size > this->freeMemAvailable)
		return nullptr;

	std::list<BlockNode*> queue;
	queue.push_back((BlockNode*)this->freeBlockTree.GetRootNode());
	while (queue.size() > 0)
	{
		auto blockNode = (BlockNode*)queue.front();
		queue.pop_front();

		auto block = (Block*)blockNode->GetKey();
		THEBE_ASSERT(block->state == Block::FREE);

		if (block->size >= size)
		{
			uint64_t offsetStart = block->offset;
			uint64_t offsetEnd = block->offset + block->size;
			uint64_t offsetAligned = THEBE_ALIGNED(offsetStart, align);
			if (offsetAligned + size <= offsetEnd)
			{
				uint64_t leftMargin = offsetAligned - offsetStart;
				uint64_t rightMargin = offsetEnd - (offsetAligned + size);

				this->freeBlockTree.RemoveNode(blockNode);

				if (leftMargin > 0)
				{
					auto newLeftBlock = new Block(this);
					newLeftBlock->offset = offsetStart;
					newLeftBlock->size = leftMargin;
					this->blockList.InsertNodeBefore(newLeftBlock, block);
					block->offset = offsetAligned;
					block->size -= leftMargin;
					auto newLeftNode = new BlockNode(newLeftBlock);
					this->freeBlockTree.InsertNode(newLeftNode);
				}

				if (rightMargin > 0)
				{
					auto newRightBlock = new Block(this);
					newRightBlock->offset = offsetAligned + size;
					newRightBlock->size = rightMargin;
					this->blockList.InsertNodeAfter(newRightBlock, block);
					block->size -= rightMargin;
					auto newRightNode = new BlockNode(newRightBlock);
					this->freeBlockTree.InsertNode(newRightNode);
				}

				THEBE_ASSERT(block->size == size);
				block->state = Block::ALLOCATED;
				this->freeMemAvailable -= size;
				return blockNode;
			}
		}

		auto leftBlockNode = (BlockNode*)blockNode->GetRightNode();
		auto rightBlockNode = (BlockNode*)blockNode->GetLeftNode();

		if (leftBlockNode && leftBlockNode->block->size >= size)
			queue.push_back(leftBlockNode);

		if (rightBlockNode && rightBlockNode->block->size >= size)
			queue.push_back(rightBlockNode);
	}

	return nullptr;
}

bool BlockManager::Deallocate(BlockNode* blockNode)
{
	Block* block = blockNode->block;

	if (block->blockManager != this || block->state != Block::ALLOCATED)
		return false;

	this->freeMemAvailable += block->size;
	block->state = Block::FREE;
	
	// If no two free blocks ever become adjacent within the heap,
	// then we should never iterate these while loops more than once.
	// Thus, I'm hopeful our time-complexity here is really O(log N).

	while (true)
	{
		auto leftBlock = (Block*)block->GetPrevNode();
		if (!leftBlock || leftBlock->state != Block::FREE)
			break;
		
		block->offset = leftBlock->offset;
		block->size += leftBlock->size;
		auto leftBlockNode = this->freeBlockTree.FindNode(leftBlock);
		THEBE_ASSERT(leftBlockNode != nullptr);
		this->freeBlockTree.RemoveNode(leftBlockNode, true);	// Remove from tree before key goes stale.
		this->blockList.RemoveNode(leftBlock, true);
	}

	while (true)
	{
		auto rightBlock = (Block*)block->GetNextNode();
		if (!rightBlock || rightBlock->state != Block::FREE)
			break;

		block->size += rightBlock->size;
		auto rightBlockNode = this->freeBlockTree.FindNode(rightBlock);
		THEBE_ASSERT(rightBlockNode != nullptr);
		this->freeBlockTree.RemoveNode(rightBlockNode, true);	// Remove from tree before key goes stale.
		this->blockList.RemoveNode(rightBlock, true);
	}

	this->freeBlockTree.InsertNode(blockNode);
	return true;
}

//------------------------------------- BlockManager::Block -------------------------------------

BlockManager::Block::Block(BlockManager* blockManager)
{
	this->blockManager = blockManager;
	this->offset = 0L;
	this->size = 0L;
	this->state = State::FREE;
}

/*virtual*/ BlockManager::Block::~Block()
{
}

/*virtual*/ bool BlockManager::Block::IsLessThan(const AVLTreeKey* key) const
{
	auto block = (const Block*)key;

	if (this->size < block->size)
		return true;

	if (this->size > block->size)
		return false;

	return this->offset < block->offset;
}

/*virtual*/ bool BlockManager::Block::IsGreaterThan(const AVLTreeKey* key) const
{
	auto block = (const Block*)key;

	if (this->size > block->size)
		return true;

	if (this->size < block->size)
		return false;

	return this->offset > block->offset;
}

/*virtual*/ bool BlockManager::Block::IsEqualto(const AVLTreeKey* key) const
{
	auto block = (const Block*)key;

	return this->size == block->size && this->offset == block->offset;
}

/*virtual*/ bool BlockManager::Block::IsNotEqualto(const AVLTreeKey* key) const
{
	return !this->IsEqualto(key);
}

//------------------------------------- BlockManager::BlockNode -------------------------------------

BlockManager::BlockNode::BlockNode(Block* block)
{
	this->block = block;
}

/*virtual*/ BlockManager::BlockNode::~BlockNode()
{
}

/*virtual*/ const AVLTreeKey* BlockManager::BlockNode::GetKey() const
{
	return this->block;
}

/*virtual*/ void BlockManager::BlockNode::SetKey(const AVLTreeKey* givenKey)
{
	this->block = (Block*)givenKey;
}