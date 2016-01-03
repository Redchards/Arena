template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::ArenaAllocationPolicy() 
noexcept(noexcept(std::declval<ArenaAllocationPolicy>().newChunk(capacity::value)))
{
	newChunk(capacity::value);
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::ArenaAllocationPolicy(const ArenaAllocationPolicy& other)
noexcept(noexcept(std::declval<ArenaAllocationPolicy>().newChunk(capacity::value)))
: ArenaAllocationPolicy()
{
	std::memcpy(chunkHandle_, other.chunkHandle_, (capacity::value * sizeof(Node_)) + padding);
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::~ArenaAllocationPolicy() noexcept
{
	static_cast<BaseAllocator>(*this).deallocate(reinterpret_cast<T*>(chunkHandle_), capacity::value);
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
T* ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::allocate(size_type, const T*) noexcept
{
	intptr_t returnedPtr = ((currentNode_->offset_));
	bool isFull = (returnedPtr == 0);
	Node_* tmp = (!isFull ? currentNode_ : nullptr);
	currentNode_ += (returnedPtr + isFull);
	
	return reinterpret_cast<T*>(tmp);
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
template<Safety type,
		 std::enable_if_t<type == Safety::Enabled>*>
void ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::deallocate(T* ptr, size_type size)
noexcept(noexcept(std::declval<ArenaAllocationPolicy>().deallocate_(ptr, size)))
{
	Node_* ptrNode = reinterpret_cast<Node_*>(ptr);
	if(((reinterpret_cast<intptr_t>(ptrNode) & alignment) == 0) 
		&& ((ptrNode >= roundUpPtr(chunkHandle_, padding)) || (ptrNode < roundUpPtr(chunkHandle_ + capacity::value, padding))))
	{
		deallocate_(ptr, size);
	}
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
template<Safety type,
		 std::enable_if_t<type == Safety::Disabled>*>
void ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::deallocate(T* ptr, size_type size)
noexcept(noexcept(std::declval<ArenaAllocationPolicy>().deallocate_(ptr, size)))
{
	deallocate_(ptr, size);
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
template<AllocationType type, 
		 std::enable_if_t<type == AllocationType::Aligned>*>
auto ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::newChunk(size_type size, const T*)
noexcept(noexcept(std::declval<BaseAllocator>().allocate(size, nullptr))) -> Node_*
{
	chunkHandle_ = reinterpret_cast<Node_*>(static_cast<BaseAllocator>(*this).allocate(size + alignmentSpace));
	currentNode_ = roundUpPtr(chunkHandle_, padding);

	std::fill_n(currentNode_, size, 1);
	
	return currentNode_;
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
template<AllocationType type, 
		 std::enable_if_t<type == AllocationType::Unaligned>*>
auto ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::newChunk(size_type size, const T*)
noexcept(noexcept(std::declval<BaseAllocator>().allocate(size, nullptr))) -> Node_*
{
	chunkHandle_ = reinterpret_cast<Node_*>(static_cast<BaseAllocator>(*this).allocate(size));

	std::fill_n(currentNode_, size, 1);
	
	return currentNode_;
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
void ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::deallocate_(T* ptr, size_type size)
noexcept(noexcept(std::declval<BaseAllocator>().deallocate(ptr, size)))
{
	Node_* ptrNode = reinterpret_cast<Node_*>(ptr);
	
	static_cast<BaseAllocator>(*this).destroy(ptr);
	ptrNode->offset_ = currentNode_ - ptrNode;
	currentNode_ = ptrNode;
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
template<AllocationType type,
		 std::enable_if_t<type == AllocationType::Aligned>*>
bool ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::equals(const ArenaAllocationPolicy& other) noexcept
{
	return (std::memcmp(roundUpPtr(chunkHandle_, padding), roundUpPtr(other.chunkHandle_, padding), capacity::value + alignmentSpace) == 0);
}

template<typename T,
		 class capacity,
		 class allocation,
		 class safety,
		 class MemoryAllocator>
template<AllocationType type,
		 std::enable_if_t<type == AllocationType::Unaligned>*>
bool ArenaAllocationPolicy<T, capacity, allocation, safety, MemoryAllocator>::equals(const ArenaAllocationPolicy& other) noexcept
{
	return (std::memcmp(chunkHandle_, other.chunkHandle_, capacity::value) == 0);
}