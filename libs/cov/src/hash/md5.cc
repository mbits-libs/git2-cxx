// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "cov/hash/md5.hh"

namespace hash {
	namespace {

		inline std::uint32_t F(std::uint32_t b,
		                       std::uint32_t c,
		                       std::uint32_t d) {
			return ((c ^ d) & b) ^ d;
		}
		inline std::uint32_t G(std::uint32_t b,
		                       std::uint32_t c,
		                       std::uint32_t d) {
			return ((b ^ c) & d) ^ c;
		}
		inline std::uint32_t H(std::uint32_t b,
		                       std::uint32_t c,
		                       std::uint32_t d) {
			return b ^ c ^ d;
		}
		inline std::uint32_t I(std::uint32_t b,
		                       std::uint32_t c,
		                       std::uint32_t d) {
			return ((~d) | b) ^ c;
		}

		inline std::uint32_t ROTATE(std::uint32_t x, int n) {
			return (x << n) | (x >> (32 - n));
		}

		template <class OP>
		inline void R_(std::uint32_t& a,
		               std::uint32_t b,
		               std::uint32_t c,
		               std::uint32_t d,
		               std::uint32_t k,
		               int s,
		               std::uint32_t t,
		               OP op) {
			a += k + t + op(b, c, d);
			a = ROTATE(a, s);
			a += b;
		}

		inline void R0(std::uint32_t& a,
		               std::uint32_t b,
		               std::uint32_t c,
		               std::uint32_t d,
		               std::uint32_t k,
		               int s,
		               std::uint32_t t) {
			R_(a, b, c, d, k, s, t, F);
		}

		inline void R1(std::uint32_t& a,
		               std::uint32_t b,
		               std::uint32_t c,
		               std::uint32_t d,
		               std::uint32_t k,
		               int s,
		               std::uint32_t t) {
			R_(a, b, c, d, k, s, t, G);
		}

		inline void R2(std::uint32_t& a,
		               std::uint32_t b,
		               std::uint32_t c,
		               std::uint32_t d,
		               std::uint32_t k,
		               int s,
		               std::uint32_t t) {
			R_(a, b, c, d, k, s, t, H);
		}

		inline void R3(std::uint32_t& a,
		               std::uint32_t b,
		               std::uint32_t c,
		               std::uint32_t d,
		               std::uint32_t k,
		               int s,
		               std::uint32_t t) {
			R_(a, b, c, d, k, s, t, I);
		}
	}  // namespace

	md5::digest_type md5::on_result() noexcept {
		digest_type result{};
		auto* it = result.data;
		l2c(A_, it);
		l2c(B_, it);
		l2c(C_, it);
		l2c(D_, it);

		A_ = 0x67452301L;
		B_ = 0xefcdab89L;
		C_ = 0x98badcfeL;
		D_ = 0x10325476L;

		return result;
	}

	void md5::transform(std::byte const* data) noexcept {
		uint32_t A = A_;
		uint32_t B = B_;
		uint32_t C = C_;
		uint32_t D = D_;

		uint32_t X[16];
		X[0] = c2l(data);
		X[1] = c2l(data);
		X[2] = c2l(data);
		X[3] = c2l(data);
		X[4] = c2l(data);
		X[5] = c2l(data);
		X[6] = c2l(data);
		X[7] = c2l(data);
		X[8] = c2l(data);
		X[9] = c2l(data);
		X[10] = c2l(data);
		X[11] = c2l(data);
		X[12] = c2l(data);
		X[13] = c2l(data);
		X[14] = c2l(data);
		X[15] = c2l(data);

		/* Round 0 */
		R0(A, B, C, D, X[0], 7, 0xd76aa478L);
		R0(D, A, B, C, X[1], 12, 0xe8c7b756L);
		R0(C, D, A, B, X[2], 17, 0x242070dbL);
		R0(B, C, D, A, X[3], 22, 0xc1bdceeeL);
		R0(A, B, C, D, X[4], 7, 0xf57c0fafL);
		R0(D, A, B, C, X[5], 12, 0x4787c62aL);
		R0(C, D, A, B, X[6], 17, 0xa8304613L);
		R0(B, C, D, A, X[7], 22, 0xfd469501L);
		R0(A, B, C, D, X[8], 7, 0x698098d8L);
		R0(D, A, B, C, X[9], 12, 0x8b44f7afL);
		R0(C, D, A, B, X[10], 17, 0xffff5bb1L);
		R0(B, C, D, A, X[11], 22, 0x895cd7beL);
		R0(A, B, C, D, X[12], 7, 0x6b901122L);
		R0(D, A, B, C, X[13], 12, 0xfd987193L);
		R0(C, D, A, B, X[14], 17, 0xa679438eL);
		R0(B, C, D, A, X[15], 22, 0x49b40821L);
		/* Round 1 */
		R1(A, B, C, D, X[1], 5, 0xf61e2562L);
		R1(D, A, B, C, X[6], 9, 0xc040b340L);
		R1(C, D, A, B, X[11], 14, 0x265e5a51L);
		R1(B, C, D, A, X[0], 20, 0xe9b6c7aaL);
		R1(A, B, C, D, X[5], 5, 0xd62f105dL);
		R1(D, A, B, C, X[10], 9, 0x02441453L);
		R1(C, D, A, B, X[15], 14, 0xd8a1e681L);
		R1(B, C, D, A, X[4], 20, 0xe7d3fbc8L);
		R1(A, B, C, D, X[9], 5, 0x21e1cde6L);
		R1(D, A, B, C, X[14], 9, 0xc33707d6L);
		R1(C, D, A, B, X[3], 14, 0xf4d50d87L);
		R1(B, C, D, A, X[8], 20, 0x455a14edL);
		R1(A, B, C, D, X[13], 5, 0xa9e3e905L);
		R1(D, A, B, C, X[2], 9, 0xfcefa3f8L);
		R1(C, D, A, B, X[7], 14, 0x676f02d9L);
		R1(B, C, D, A, X[12], 20, 0x8d2a4c8aL);
		/* Round 2 */
		R2(A, B, C, D, X[5], 4, 0xfffa3942L);
		R2(D, A, B, C, X[8], 11, 0x8771f681L);
		R2(C, D, A, B, X[11], 16, 0x6d9d6122L);
		R2(B, C, D, A, X[14], 23, 0xfde5380cL);
		R2(A, B, C, D, X[1], 4, 0xa4beea44L);
		R2(D, A, B, C, X[4], 11, 0x4bdecfa9L);
		R2(C, D, A, B, X[7], 16, 0xf6bb4b60L);
		R2(B, C, D, A, X[10], 23, 0xbebfbc70L);
		R2(A, B, C, D, X[13], 4, 0x289b7ec6L);
		R2(D, A, B, C, X[0], 11, 0xeaa127faL);
		R2(C, D, A, B, X[3], 16, 0xd4ef3085L);
		R2(B, C, D, A, X[6], 23, 0x04881d05L);
		R2(A, B, C, D, X[9], 4, 0xd9d4d039L);
		R2(D, A, B, C, X[12], 11, 0xe6db99e5L);
		R2(C, D, A, B, X[15], 16, 0x1fa27cf8L);
		R2(B, C, D, A, X[2], 23, 0xc4ac5665L);
		/* Round 3 */
		R3(A, B, C, D, X[0], 6, 0xf4292244L);
		R3(D, A, B, C, X[7], 10, 0x432aff97L);
		R3(C, D, A, B, X[14], 15, 0xab9423a7L);
		R3(B, C, D, A, X[5], 21, 0xfc93a039L);
		R3(A, B, C, D, X[12], 6, 0x655b59c3L);
		R3(D, A, B, C, X[3], 10, 0x8f0ccc92L);
		R3(C, D, A, B, X[10], 15, 0xffeff47dL);
		R3(B, C, D, A, X[1], 21, 0x85845dd1L);
		R3(A, B, C, D, X[8], 6, 0x6fa87e4fL);
		R3(D, A, B, C, X[15], 10, 0xfe2ce6e0L);
		R3(C, D, A, B, X[6], 15, 0xa3014314L);
		R3(B, C, D, A, X[13], 21, 0x4e0811a1L);
		R3(A, B, C, D, X[4], 6, 0xf7537e82L);
		R3(D, A, B, C, X[11], 10, 0xbd3af235L);
		R3(C, D, A, B, X[2], 15, 0x2ad7d2bbL);
		R3(B, C, D, A, X[9], 21, 0xeb86d391L);

		A_ += A;
		B_ += B;
		C_ += C;
		D_ += D;
	}
}  // namespace hash
