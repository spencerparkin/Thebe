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
	this->freeBlockTree.Clear(false);
	this->blockList.Clear(true);

	this->freeMemAvailable = size;

	if (size > 0)
	{
		auto blockNode = new BlockNode();
		blockNode->blockManager = this;
		blockNode->key.offset = 0;
		blockNode->key.size = size;
		this->blockList.InsertNodeAfter(blockNode);
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

		THEBE_ASSERT(blockNode->key.state == BlockKey::FREE);

		uint64_t blockSize = blockNode->GetSize();
		if (blockSize >= size)
		{
			uint64_t offsetStart = blockNode->GetOffset();
			uint64_t offsetEnd = blockNode->GetOffset() + blockSize;
			uint64_t offsetAligned = THEBE_ALIGNED(offsetStart, align);
			if (offsetAligned + size <= offsetEnd)
			{
				uint64_t leftMargin = offsetAligned - offsetStart;
				uint64_t rightMargin = offsetEnd - (offsetAligned + size);

				this->freeBlockTree.RemoveNode(blockNode);

				if (leftMargin > 0)
				{
					auto newLeftNode = new BlockNode();
					newLeftNode->blockManager = this;
					newLeftNode->key.offset = offsetStart;
					newLeftNode->key.size = leftMargin;
					this->blockList.InsertNodeBefore(newLeftNode, blockNode);
					blockNode->key.offset = offsetAligned;
					blockNode->key.size -= leftMargin;
					this->freeBlockTree.InsertNode(newLeftNode);
				}

				if (rightMargin > 0)
				{
					auto newRightNode = new BlockNode();
					newRightNode->blockManager = this;
					newRightNode->key.offset = offsetAligned + size;
					newRightNode->key.size = rightMargin;
					this->blockList.InsertNodeAfter(newRightNode, blockNode);
					blockNode->key.size -= rightMargin;
					this->freeBlockTree.InsertNode(newRightNode);
				}

				THEBE_ASSERT(blockNode->GetSize() == size);
				blockNode->key.state = BlockKey::ALLOCATED;
				this->freeMemAvailable -= blockNode->GetSize();
				return blockNode;
			}
		}

		auto leftBlockNode = (BlockNode*)blockNode->GetRightNode();
		auto rightBlockNode = (BlockNode*)blockNode->GetLeftNode();

		if (leftBlockNode && leftBlockNode->GetSize() >= size)
			queue.push_back(leftBlockNode);

		if (rightBlockNode && rightBlockNode->GetSize() >= size)
			queue.push_back(rightBlockNode);
	}

	return nullptr;
}

bool BlockManager::Deallocate(BlockNode* blockNode)
{
	if (blockNode->blockManager != this)
		return false;

	if (blockNode->key.state != BlockKey::ALLOCATED)
		return false;

	this->freeMemAvailable += blockNode->key.size;
	blockNode->key.state = BlockKey::FREE;
	
	// If no two free blocks ever become adjacent within the heap,
	// then we should never iterate these while loops more than once.
	// Thus, I'm hopeful our time-complexity here is O(log N).

	while (true)
	{
		auto leftBlockNode = (BlockNode*)blockNode->GetPrevNode();
		if (!leftBlockNode || leftBlockNode->key.state != BlockKey::FREE)
			break;
		
		this->freeBlockTree.RemoveNode(leftBlockNode);
		blockNode->key.offset = leftBlockNode->key.offset;
		blockNode->key.size += leftBlockNode->key.size;
		this->blockList.RemoveNode(leftBlockNode, true);
	}

	while (true)
	{
		auto rightBlockNode = (BlockNode*)blockNode->GetNextNode();
		if (!rightBlockNode || rightBlockNode->key.state != BlockKey::FREE)
			break;

		this->freeBlockTree.RemoveNode(rightBlockNode);
		blockNode->key.size += rightBlockNode->key.size;
		this->blockList.RemoveNode(rightBlockNode, true);
	}

	this->freeBlockTree.InsertNode(blockNode);
	
	return true;
}

//------------------------------------- BlockManager::BlockKey -------------------------------------

BlockManager::BlockKey::BlockKey()
{
	this->offset = 0L;
	this->size = 0L;
	this->state = State::FREE;
}

/*virtual*/ BlockManager::BlockKey::~BlockKey()
{
}

/*virtual*/ bool BlockManager::BlockKey::IsLessThan(const AVLTreeKey* key) const
{
	auto blockKey = (const BlockKey*)key;

	if (this->size < blockKey->size)
		return true;

	if (this->size > blockKey->size)
		return false;

	return this->offset < blockKey->offset;
}

/*virtual*/ bool BlockManager::BlockKey::IsGreaterThan(const AVLTreeKey* key) const
{
	auto blockKey = (const BlockKey*)key;

	if (this->size > blockKey->size)
		return true;

	if (this->size < blockKey->size)
		return false;

	return this->offset > blockKey->offset;
}

/*virtual*/ bool BlockManager::BlockKey::IsEqualto(const AVLTreeKey* key) const
{
	auto blockKey = (const BlockKey*)key;

	return this->size == blockKey->size && this->offset == blockKey->offset;
}

/*virtual*/ bool BlockManager::BlockKey::IsNotEqualto(const AVLTreeKey* key) const
{
	return !this->IsEqualto(key);
}

//------------------------------------- BlockManager::BlockNode -------------------------------------

BlockManager::BlockNode::BlockNode()
{
	this->blockManager = nullptr;
}

/*virtual*/ BlockManager::BlockNode::~BlockNode()
{
}

/*virtual*/ const AVLTreeKey* BlockManager::BlockNode::GetKey() const
{
	return &this->key;
}

/*virtual*/ void BlockManager::BlockNode::SetKey(const AVLTreeKey* givenKey)
{
	auto blockKey = (const BlockKey*)givenKey;
	this->key.offset = blockKey->offset;
	this->key.size = blockKey->size;
}

uint64_t BlockManager::BlockNode::GetOffset() const
{
	return this->key.offset;
}

uint64_t BlockManager::BlockNode::GetSize() const
{
	return this->key.size;
}