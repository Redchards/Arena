#ifndef META_MINIMAL_HXX
#define META_MINIMAL_HXX

// A very very small subset of my already small template metaprogramming library.

#include <CommonTypes.hxx>

struct neutral {};

template<class ...>
using void_t = void;

template<class T>
struct identity
{
	using type = T;
};

template<class MetaFunction>
using invoke = typename MetaFunction::type;

template<uint8 byteNumber>
struct sized_integer;

template<>
struct sized_integer<1> : identity<uint8> {};

template<>
struct sized_integer<2> : identity<uint16> {};

template<>
struct sized_integer<4> : identity<uint32> {};

template<>
struct sized_integer<8> : identity<uint64> {};

template<size_t byteNumber>
using sized_integer_t = invoke<sized_integer<byteNumber>>;

// This metafunction do power of two roundup
template<size_t num, size_t pow2>
struct roundUp
{
	static constexpr size_t mask = ((1 << pow2) - 1);
	static constexpr size_t value = ((num + mask) & ~mask);
};

template<size_t num>
struct roundUp<num, 0>
{
	static constexpr size_t value = num;
};

template<size_t num>
struct roundUp<num, sizeof(size_t)*8>
{
	static constexpr size_t mask = ~(std::numeric_limits<size_t>::max() >> 1);
	static constexpr size_t value = (num & ~(std::numeric_limits<size_t>::max() >> 1));	
};

#endif // META_MINIMAL_HXX