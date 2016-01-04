#ifndef ALLOCATOR
#define ALLOCATOR

#include <MetaMinimal.hxx>

#include <cstdlib>
#include <limits>
#include <type_traits>
#include <utility>

#define DEFAULT_ALIGN 1

template<class, class = void>
struct has_rebind : std::false_type {};

template<class Allocator>
struct has_rebind<Allocator, void_t<decltype(Allocator::template rebind<neutral>)>> : std::true_type {};

template<class, class Default, class = void>
struct defaulted_size_type 
: identity<Default> {};

template<class AllocationPolicy, class Default>
struct defaulted_size_type<AllocationPolicy, Default, void_t<decltype(typename AllocationPolicy::size_type{})>>
: identity<typename AllocationPolicy::size_type> {};

template<class, class Default, class = void>
struct defaulted_difference_type 
: identity<Default> {};

template<class AllocationPolicy, class Default>
struct defaulted_difference_type<AllocationPolicy, Default, void_t<decltype(typename AllocationPolicy::difference_type{})>>
: identity<typename AllocationPolicy::difference_type> {};

template<template<class...> class AllocationPolicy, typename U, class... PolicyArgs>
struct rebind_allocator
{
	typedef AllocationPolicy<U, PolicyArgs...> type;
};

template<template<class...> class AllocationPolicy, typename U, class ... PolicyArgs>
using rebind_allocator_t = typename rebind_allocator<AllocationPolicy, U, PolicyArgs...>::type;

/*
	Very simple allocator creation helper.
*/
template<typename T,
		 template<class...> class AllocationPolicy,
		 class ... PolicyArgs>
class Allocator : public AllocationPolicy<T, PolicyArgs...>
{
	template<class TAllocationPolicy, class = void>
	struct get_alignment 
	: std::integral_constant<size_t, DEFAULT_ALIGN> {};

	template<class TAllocationPolicy>
	struct get_alignment<TAllocationPolicy, void_t<decltype(TAllocationPolicy::alignment)>>
	: std::integral_constant<size_t, TAllocationPolicy::alignment> {};
	
	public:
	typedef AllocationPolicy<T, PolicyArgs...> Policy;
	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef typename defaulted_size_type<Policy, size_t>::type size_type;
	typedef typename defaulted_difference_type<Policy, ptrdiff_t>::type difference_type;
	
	public:
	static constexpr uint64 alignment = get_alignment<Policy>::value;
		
	using Policy::Policy;
	
	T* allocate(size_type size, const_pointer hint = nullptr)
	{
		//std::cout << CTTI::GetTypeName<Policy>() << std::endl;
		return Policy::allocate(size, hint);
	}
	
	void deallocate(pointer ptr, size_type size)
	{
		Policy::deallocate(ptr, size);
	}
	
	template<class... Args>
	void construct(pointer ptr, Args... args)
	{
		new(ptr) value_type(std::forward<Args>(args)...);
	}
	
	void destroy(pointer ptr)
	{
		ptr->~value_type();
	}

	template<typename T1, typename T2,
			 template<class ...> class TAllocationPolicy,
			 class ... TPolicyArgs>
	friend bool operator==(const Allocator<T1, TAllocationPolicy, TPolicyArgs...>& lhs,
	  					   const Allocator<T2, TAllocationPolicy, TPolicyArgs...>& rhs);
	
	template<typename T1, typename T2,
			 template<class ...> class TAllocationPolicy,
			 class ... TPolicyArgs>
	friend bool operator!=(const Allocator<T1, AllocationPolicy, PolicyArgs...>& lhs,
						   const Allocator<T2, AllocationPolicy, PolicyArgs...>& rhs);
	
	constexpr inline size_type max_size() const
	{ 
        return (std::numeric_limits<size_type>::max() / sizeof(T)); 
    }
    
    pointer address(reference ref) const
    {
    	return std::addressof(ref);
    }
    
    const_pointer address(const_reference ref) const
    {
    	return std::addressof(ref);
    }
    
    template<typename U>
	struct rebind
	{
		using other = Allocator<U, AllocationPolicy, PolicyArgs...>;
	};
};

template<typename T1, typename T2,
		 template<class ...> class TAllocationPolicy,
		 class ... TPolicyArgs>
bool operator==(const Allocator<T1, TAllocationPolicy, TPolicyArgs...>& lhs,
	  		    const Allocator<T2, TAllocationPolicy, TPolicyArgs...>& rhs)
{
	return lhs.equals(rhs);
}

template<typename T1, typename T2,
		 template<class ...> class TAllocationPolicy,
		 class ... TPolicyArgs>
bool operator!=(const Allocator<T1, TAllocationPolicy, TPolicyArgs...>& lhs,
				const Allocator<T2, TAllocationPolicy, TPolicyArgs...>& rhs)
{
	return !(lhs == rhs);					
}

/*
	Default allocation policy. Only call the native memory allocation functions.
	NOTE : using operator new[]() and operator delete[]() could be wiser here than using malloc and free.
*/
template<typename T>
class DefaultAllocationPolicy
{	
	public:
	explicit DefaultAllocationPolicy() noexcept
	{}
	inline explicit DefaultAllocationPolicy(const DefaultAllocationPolicy&) noexcept
	{}
	template<typename U>
	inline explicit DefaultAllocationPolicy(const DefaultAllocationPolicy<U>& other) noexcept
	{
		DefaultAllocationPolicy(static_cast<rebind_allocator_t<::DefaultAllocationPolicy, T>>(other));
	}
	
	~DefaultAllocationPolicy(){}
	
	T* allocate(size_t size, const T* = nullptr) const noexcept
	{
		return static_cast<T*>(malloc(size * sizeof(T)));
	}
	
	void deallocate(T* ptr, size_t) const noexcept
	{
		free(ptr);
	}
	
	template<typename U>
	bool equals(const DefaultAllocationPolicy<U>&) const noexcept
	{
		return true;
	}
};

template<typename T>
using DefaultAllocator = Allocator<T, DefaultAllocationPolicy>;

#endif // ALLOCATOR
