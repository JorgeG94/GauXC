#pragma once
#include <cstdint>
#include <stdlib.h>
#include <type_traits>

namespace GauXC {

template <uint32_t N, typename T>
inline constexpr T integral_pow( T x ) {
  if constexpr ( N == 0 ) return T(1);
  if constexpr ( N == 1 ) return x;
  else                    return x * integral_pow<N-1>(x);
  abort(); // Unreachable
}

template <uint64_t N>
struct integral_pow_two : std::integral_constant< uint64_t, (1ul << N) > {};

template <uint64_t N>
struct integral_factorial;
template<>
struct integral_factorial<0ul> : std::integral_constant< uint64_t, 1ul > {};
template <uint64_t N>
struct integral_factorial : 
  std::integral_constant< uint64_t, N * integral_factorial<N-1>::value > {};

namespace constants {

template <typename T = double>
inline constexpr T pi = 3.14159265358979323846;
template <typename T = double>
inline constexpr T sqrt_pi = 1.77245385090551602729;
template <typename T = double>
inline constexpr T sqrt_pi_ov_2 = 0.88622692545275801364;

}

inline constexpr double rsqrt( double x ) {
  double y = x;
  double x2 = y * 0.5;
  int64_t i = *(int64_t*)&y;
  i = 0x5fe6eb50c7b537a9 - (i >> 1);
  y = *(double *) &i;
  y = y * (1.5 - (x2 * y * y));
  y = y * (1.5 - (x2 * y * y));
  return y;
}

}
