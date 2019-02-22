#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "strings.hpp"

namespace Util {
	// Clamps a numeric type to a narrower numeric type.
	template<typename T, typename U>
	constexpr T ClampTo(U value)
	{
		return static_cast<T>(std::clamp<U>(value, (std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)()));
	}

	// Converts between bit representations. Superseded in C++20 by std::bit_cast.
	template<typename T, typename F>
	inline T WordCast(F v)
	{
		static_assert(sizeof(T) == sizeof(F), "Sizes do not match.");
		static_assert(std::is_trivially_copyable_v<T>, "T is not trivially copyable.");
		static_assert(std::is_trivially_copyable_v<F>, "F is not trivially copyable.");

		T ret;
		std::memcpy(&ret, &v, sizeof(T));
		return ret;
	}

	namespace impl {
		constexpr bool IsDecimalDigit(wchar_t character)
		{
			return character >= L'0' && character <= L'9';
		}

		constexpr bool IsCapitalHexDigit(wchar_t character)
		{
			return character >= L'A' && character <= L'F';
		}

		constexpr bool IsLowerHexDigit(wchar_t character)
		{
			return character >= L'a' && character <= L'f';
		}

		template<typename T>
		constexpr T pow(uint8_t base, std::size_t exponent)
		{
			if (exponent == 0)
			{
				return 1;
			}
			else
			{
				T result = base;
				for (std::size_t i = exponent - 1; i > 0; i--)
				{
					result *= base;
				}
				return result;
			}
		}

		template<typename T, uint8_t base>
		struct NumberParser;

		template<typename T>
		struct NumberParser<T, 10> {
			static constexpr T impl(std::wstring_view number)
			{
				bool isNegative = false;
				if constexpr (std::is_signed_v<T>)
				{
					if (number.length() > 1 && number[0] == L'-')
					{
						number.remove_prefix(1);
						isNegative = true;
					}
				}

				T result {};
				for (std::size_t i = number.length() - 1; i >= 0; i--)
				{
					if (impl::IsDecimalDigit(number[i]))
					{
						const T power = impl::pow<T>(10, number.length() - i - 1);
						result += (number[i] - L'0') * power;
					}
					else
					{
						throw std::invalid_argument("Not a number");
					}
				}

				if constexpr (std::is_signed_v<T>)
				{
					return isNegative ? -result : result;
				}
				else
				{
					return result;
				}
			}
		};

		template<typename T>
		struct NumberParser<T, 16> {
			static_assert (std::is_unsigned_v<T>, "T must be unsigned to parse in base 16");

			static constexpr T impl(std::wstring_view number)
			{
				if (number.length() > 2 && number[0] == L'0' && (number[1] == L'x' || number[1] == L'X'))
				{
					number.remove_prefix(2);
				}

				T result {};
				for (std::size_t i = number.length() - 1; i >= 0; i--)
				{
					const T power = 1 << ((number.length() - i - 1) * 4);
					if (impl::IsDecimalDigit(number[i]))
					{
						result += (number[i] - L'0') * power;
					}
					else if (impl::IsCapitalHexDigit(number[i]))
					{
						result += (number[i] - L'A' + 10) * power;
					}
					else if (impl::IsLowerHexDigit(number[i]))
					{
						result += (number[i] - L'a' + 10) * power;
					}
					else
					{
						throw std::invalid_argument("Not a number");
					}
				}

				return result;
			}
		};
	}

	// Apparently no wide string to number parser accepted an explicit ending to the string
	// so here I am. Also C locales sucks.
	template<typename T = int32_t, uint8_t base = 10>
	constexpr T ParseNumber(std::wstring_view number)
	{
		static_assert(std::is_integral_v<T>, "T must be an integral type");

		return impl::NumberParser<T, base>::impl(number);
	}
}