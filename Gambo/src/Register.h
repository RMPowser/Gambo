#pragma once

#include <bitset>
#include <concepts>
#include <type_traits>

template<size_t bitWidth>
	requires (bitWidth > 0 && bitWidth <= sizeof(size_t) * 8)
class Register
{
	using SmallestIntegerType =
		std::conditional_t<(bitWidth <= 8), uint8_t,
			std::conditional_t<(bitWidth <= 16), uint16_t,
				std::conditional_t<(bitWidth <= 32), uint32_t,
					uint64_t>>>;

	using ThisType = Register<bitWidth>;

	std::bitset<bitWidth> bits;

public:
	Register()
		: bits(0)
	{
	}

	Register(const size_t& n)
	{
		operator=(n);
	}

	size_t GetBits(const size_t& bitIndex, const size_t& bitMask = 0b1)
	{
		return (*this & (bitMask << bitIndex)) >> bitIndex;
	}

	ThisType& SetBit(const size_t& bitIndex, const bool b)
	{
		bits.set(bitIndex, b);
		return *this;
	}

	// implicit conversion function to smallest type that can store bitWidth bits
	operator SmallestIntegerType()
	{
		return static_cast<SmallestIntegerType>(bits.to_ullong());
	}

	ThisType& operator++() // prefix
	{
		bits = bits.to_ullong() + 1;
		return *this;
	}

	ThisType operator++(int) // postfix
	{
		ThisType temp = *this;
		++(*this);
		return temp;
	}

	ThisType& operator--() // prefix
	{
		bits = bits.to_ullong() - 1;
		return *this;
	}

	ThisType operator--(int) // postfix
	{
		ThisType temp = *this;
		--(*this);
		return temp;
	}

	ThisType& operator=(const size_t& n)
	{
		for (size_t i = 0; i < sizeof(n) * 8 && i < bitWidth; i++)
		{
			if (n & (1ull << i))
			{
				bits.set(i);
			}
			else
			{
				bits.reset(i);
			}
		}
		return *this;
	}

	friend float operator+(float f, ThisType r)
	{
		return f + r.bits.to_ullong();
	}

	friend float operator-(float f, ThisType r)
	{
		return f - r.bits.to_ullong();
	}

	friend float operator*(float f, ThisType r)
	{
		return f * r.bits.to_ullong();
	}

	friend float operator/(float f, ThisType r)
	{
		return f / r.bits.to_ullong();
	}
	
	ThisType& operator+=(const size_t& i)
	{
		bits = bits.to_ullong() + i;
		return *this;
	}

	ThisType& operator-=(const size_t& i)
	{
		bits = bits.to_ullong() - i;
		return *this;
	}

	ThisType& operator>>=(const size_t& i)
	{
		bits >>= 1;
		return *this;
	}

	ThisType& operator<<=(const size_t & i)
	{
		bits <<= 1;
		return *this;
	}

	ThisType operator&(const size_t& n)
	{
		return bits.to_ullong() & n;
	}
};