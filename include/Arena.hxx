#ifndef ARENA_ALLOCATOR_HXX
#define ARENA_ALLOCATOR_HXX

#include <Allocator.hxx>
#include <MetaMinimal.hxx>
//#include <Variant.hxx>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>
#include <utility>
#include <cmath>
#include <cstdlib>

/*
	These flags are used to change the allocation type of the arena allocator.
	Aligned will guarantee that the memory will always be aligned, but consume a bit more space
	(it allocate the space to perform the alignment), and can be less performant, due to memory 
	roundup, if the memory is already aligned.
	
	Default : Aligned
*/
enum class AllocationType : uint8
{
	Aligned,
	Unaligned
};

/*
	These flags are used to turn the safety on and off. The safety mod add some checks here and there,
	and doing so incure an extremly tiny performance penalty.
	
	Default : Safe
*/
enum class Safety : uint8
{
	Enabled,
	Disabled
};

/*
	Performs memory address roundup to the closest "alignment" value, using the well known roundup algorithm
*/
template<class T>
T* roundUpPtr(T* ptr, uint32 alignment)
{
	size_t mask = alignment != 0 ? alignment - 1 : 0;
	return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(ptr) + mask) & ~mask);
}

/*
	Arena allocator policy class. And arena allocator is basically only a fixed pool allocator, using pointer bumping
	technic. The allocator is also optimized for object which sizeof(T) < sizeof(T*) (like 16bits types, i.e int16,
	in 64bits computers).
	
	Args : 
	- T = The type to allocate
	- capacity = an integral constant, describing the capacity of the arena
	- allocation = an integral constant, describing the allocation type (AllocationType)
	- safety = an integral constant, describing the safety mod (Safety)
	- MemoryAllocator = the base memory allocator, used to allocate/deallocate the main chunk, construct/destruct objects
*/
template<typename T,
		 class capacity = std::integral_constant<size_t, 4096>,
		 class allocation = std::integral_constant<AllocationType, AllocationType::Aligned>,
		 class safety = std::integral_constant<Safety, Safety::Enabled>,
		 class MemoryAllocator = DefaultAllocator<T>>
class ArenaAllocationPolicy : public MemoryAllocator::template rebind<T>::other
{
	static_assert(capacity::value > 0, 
				  "Arena allocator with capacity of 0 is invalid !");
	
	protected:
	union Node_;
	using BaseAllocator = typename MemoryAllocator::template rebind<T>::other;
	
	static constexpr uint64 alignment = std::conditional<allocation::value == AllocationType::Aligned,
														  std::integral_constant<uint64, alignof(T)>,
														  std::integral_constant<uint64, 0>>::type::value;

	static constexpr uint64 padding = (alignment > BaseAllocator::alignment ? alignment : 0);
	const uint64 alignmentSpace = std::ceil(static_cast<float>(alignment)/sizeof(T));

	public:
	typedef size_t size_type;
	typedef ptrdiff difference_type;
	typedef uint64* uint_ptr;
	

	public:
	ArenaAllocationPolicy() noexcept(noexcept(std::declval<ArenaAllocationPolicy>().newChunk(capacity::value)));
	explicit ArenaAllocationPolicy(const ArenaAllocationPolicy& other) noexcept(noexcept(std::declval<ArenaAllocationPolicy>().newChunk(capacity::value)));
	~ArenaAllocationPolicy() noexcept;

	/*
		Allocate an object, and return a pointer to it.
	*/
	T* allocate(size_type size, const T* hint = nullptr) noexcept;
	
	/* 
		Safety mode enabled.
   		To provide a strong ownership guarantee, we use the fact that alignment is always a power of two, so
   		we can use some bit-wise operation to perform the modulus. 
	*/
	template<Safety type = safety::value,
			 std::enable_if_t<type == Safety::Enabled>* = nullptr>
	void deallocate(T* ptr, size_type size) noexcept(noexcept(std::declval<ArenaAllocationPolicy>().deallocate_(ptr, size)));
	/* 
		Safety mode enabled.
   		No check performed, and the object is destroyed immediatly.
	*/
	template<Safety type = safety::value,
			 std::enable_if_t<type == Safety::Disabled>* = nullptr>
	void deallocate(T* ptr, size_type size) noexcept(noexcept(std::declval<ArenaAllocationPolicy>().deallocate_(ptr, size)));

	protected:
	/*
		Aligned allocations.
		Allocates a new chunk of memory. The returned pointer is correctly aligned.
	*/
	template<AllocationType type = allocation::value,
			 std::enable_if_t<type == AllocationType::Aligned>* = nullptr>
	Node_* newChunk(size_type size, const T* hint = nullptr) noexcept(noexcept(std::declval<BaseAllocator>().allocate(size, hint)));
	/*
		Unaligned allocations.
		Allocates a new chunk of memory. The returned pointer is not guaranteed to be aligned.
	*/
	template<AllocationType type = allocation::value,
			 std::enable_if_t<type == AllocationType::Unaligned>* = nullptr>
	Node_* newChunk(size_type size, const T* hint = nullptr) noexcept(noexcept(std::declval<BaseAllocator>().allocate(size, hint)));

	void deallocate_(T* ptr, size_type size) noexcept(noexcept(std::declval<BaseAllocator>().deallocate(ptr, size)));

	/* 
		Function used by the allocator creation helper to generate the operator== and operator!= definition
	*/
	template<AllocationType type = (padding == 0 ? AllocationType::Unaligned : allocation::value),
			 std::enable_if_t<type == AllocationType::Aligned>* = nullptr>
	bool equals(const ArenaAllocationPolicy& other) noexcept;
	template<AllocationType type = (padding == 0 ? AllocationType::Unaligned : allocation::value),
			 std::enable_if_t<type == AllocationType::Unaligned>* = nullptr>
	bool equals(const ArenaAllocationPolicy& other) noexcept;
	
	Node_* currentNode_;
	Node_* chunkHandle_;

	union Node_
	{
		T object_;
		static constexpr size_t typeSize = ((sizeof(T) < sizeof(intptr_t) ? sizeof(T) : sizeof(intptr_t)));
		sized_integer_t<(typeSize != 1 ? roundUp<typeSize, 1>::value : typeSize)> offset_;

		Node_& operator=(const intptr_t rhs)
		{
			offset_ = rhs;
			return *this;
		}
	};
};

/*
	The actual allocator type, using the allocator creation helper
*/
template<typename T,
		 size_t capacity = 4096,
		 AllocationType allocation = AllocationType::Aligned,
		 Safety safety = Safety::Enabled,
		 class MemoryAllocator = DefaultAllocator<T>>
using ArenaAllocator = Allocator<T,
								 ArenaAllocationPolicy,
								 std::integral_constant<size_t, capacity>,
								 std::integral_constant<AllocationType, allocation>,
								 std::integral_constant<Safety, safety>,
								 MemoryAllocator>;

#include <Arena.txx>

#endif // ARENA_ALLOCATOR_HXX