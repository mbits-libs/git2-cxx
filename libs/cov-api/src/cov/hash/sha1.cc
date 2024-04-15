// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "cov/hash/sha1.hh"
#include <cstdint>

namespace hash {
	namespace {
		inline std::uint32_t ROTATE(std::uint32_t x, int n) {
			return (x << n) | (x >> (32 - n));
		}

		void SHA_ROUND(uint32_t const W,
		               std::uint32_t& a,
		               std::uint32_t& b,
		               std::uint32_t& c,
		               std::uint32_t& d,
		               std::uint32_t& e,
		               std::uint32_t const bcd,
		               std::uint32_t const K) {
			auto const temp = ROTATE(a, 5) + bcd + e + W + K;
			e = d;
			d = c;
			c = ROTATE(b, 30);
			b = a;
			a = temp;
		}

		void T_0_19(uint32_t const W,
		            std::uint32_t& a,
		            std::uint32_t& b,
		            std::uint32_t& c,
		            std::uint32_t& d,
		            std::uint32_t& e) {
			SHA_ROUND(W, a, b, c, d, e, (b & c) | ((~b) & d), 0x5A827999);
		}

		void T_20_39(uint32_t const W,
		             std::uint32_t& a,
		             std::uint32_t& b,
		             std::uint32_t& c,
		             std::uint32_t& d,
		             std::uint32_t& e) {
			SHA_ROUND(W, a, b, c, d, e, b ^ c ^ d, 0x6ED9EBA1);
		}

		void T_40_59(uint32_t const W,
		             std::uint32_t& a,
		             std::uint32_t& b,
		             std::uint32_t& c,
		             std::uint32_t& d,
		             std::uint32_t& e) {
			SHA_ROUND(W, a, b, c, d, e, (b & c) | (b & d) | (c & d),
			          0x8F1BBCDC);
		}

		void T_60_79(uint32_t const W,
		             std::uint32_t& a,
		             std::uint32_t& b,
		             std::uint32_t& c,
		             std::uint32_t& d,
		             std::uint32_t& e) {
			SHA_ROUND(W, a, b, c, d, e, b ^ c ^ d, 0xCA62C1D6);
		}
	}  // namespace

	sha1::digest_type sha1::on_result() noexcept {
		digest_type result{};
		auto* it = result.data;
		l2c(A_, it);
		l2c(B_, it);
		l2c(C_, it);
		l2c(D_, it);
		l2c(E_, it);

		A_ = 0x67452301L;
		B_ = 0xEFCDAB89L;
		C_ = 0x98BADCFEL;
		D_ = 0x10325476L;
		E_ = 0xC3D2E1F0L;

		return result;
	}

	void sha1::transform(std::byte const* data) noexcept {
		uint32_t A = A_;
		uint32_t B = B_;
		uint32_t C = C_;
		uint32_t D = D_;
		uint32_t E = E_;

		uint32_t W[80];
		for (size_t t = 0; t < 16; ++t)
			W[t] = c2l(data);

		for (size_t t = 16; t < 80; ++t)
			W[t] = ROTATE(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);

		T_0_19(W[0], A, B, C, D, E);
		T_0_19(W[1], A, B, C, D, E);
		T_0_19(W[2], A, B, C, D, E);
		T_0_19(W[3], A, B, C, D, E);
		T_0_19(W[4], A, B, C, D, E);
		T_0_19(W[5], A, B, C, D, E);
		T_0_19(W[6], A, B, C, D, E);
		T_0_19(W[7], A, B, C, D, E);
		T_0_19(W[8], A, B, C, D, E);
		T_0_19(W[9], A, B, C, D, E);
		T_0_19(W[10], A, B, C, D, E);
		T_0_19(W[11], A, B, C, D, E);
		T_0_19(W[12], A, B, C, D, E);
		T_0_19(W[13], A, B, C, D, E);
		T_0_19(W[14], A, B, C, D, E);
		T_0_19(W[15], A, B, C, D, E);
		T_0_19(W[16], A, B, C, D, E);
		T_0_19(W[17], A, B, C, D, E);
		T_0_19(W[18], A, B, C, D, E);
		T_0_19(W[19], A, B, C, D, E);

		T_20_39(W[20], A, B, C, D, E);
		T_20_39(W[21], A, B, C, D, E);
		T_20_39(W[22], A, B, C, D, E);
		T_20_39(W[23], A, B, C, D, E);
		T_20_39(W[24], A, B, C, D, E);
		T_20_39(W[25], A, B, C, D, E);
		T_20_39(W[26], A, B, C, D, E);
		T_20_39(W[27], A, B, C, D, E);
		T_20_39(W[28], A, B, C, D, E);
		T_20_39(W[29], A, B, C, D, E);
		T_20_39(W[30], A, B, C, D, E);
		T_20_39(W[31], A, B, C, D, E);
		T_20_39(W[32], A, B, C, D, E);
		T_20_39(W[33], A, B, C, D, E);
		T_20_39(W[34], A, B, C, D, E);
		T_20_39(W[35], A, B, C, D, E);
		T_20_39(W[36], A, B, C, D, E);
		T_20_39(W[37], A, B, C, D, E);
		T_20_39(W[38], A, B, C, D, E);
		T_20_39(W[39], A, B, C, D, E);

		T_40_59(W[40], A, B, C, D, E);
		T_40_59(W[41], A, B, C, D, E);
		T_40_59(W[42], A, B, C, D, E);
		T_40_59(W[43], A, B, C, D, E);
		T_40_59(W[44], A, B, C, D, E);
		T_40_59(W[45], A, B, C, D, E);
		T_40_59(W[46], A, B, C, D, E);
		T_40_59(W[47], A, B, C, D, E);
		T_40_59(W[48], A, B, C, D, E);
		T_40_59(W[49], A, B, C, D, E);
		T_40_59(W[50], A, B, C, D, E);
		T_40_59(W[51], A, B, C, D, E);
		T_40_59(W[52], A, B, C, D, E);
		T_40_59(W[53], A, B, C, D, E);
		T_40_59(W[54], A, B, C, D, E);
		T_40_59(W[55], A, B, C, D, E);
		T_40_59(W[56], A, B, C, D, E);
		T_40_59(W[57], A, B, C, D, E);
		T_40_59(W[58], A, B, C, D, E);
		T_40_59(W[59], A, B, C, D, E);

		T_60_79(W[60], A, B, C, D, E);
		T_60_79(W[61], A, B, C, D, E);
		T_60_79(W[62], A, B, C, D, E);
		T_60_79(W[63], A, B, C, D, E);
		T_60_79(W[64], A, B, C, D, E);
		T_60_79(W[65], A, B, C, D, E);
		T_60_79(W[66], A, B, C, D, E);
		T_60_79(W[67], A, B, C, D, E);
		T_60_79(W[68], A, B, C, D, E);
		T_60_79(W[69], A, B, C, D, E);
		T_60_79(W[70], A, B, C, D, E);
		T_60_79(W[71], A, B, C, D, E);
		T_60_79(W[72], A, B, C, D, E);
		T_60_79(W[73], A, B, C, D, E);
		T_60_79(W[74], A, B, C, D, E);
		T_60_79(W[75], A, B, C, D, E);
		T_60_79(W[76], A, B, C, D, E);
		T_60_79(W[77], A, B, C, D, E);
		T_60_79(W[78], A, B, C, D, E);
		T_60_79(W[79], A, B, C, D, E);

		A_ += A;
		B_ += B;
		C_ += C;
		D_ += D;
		E_ += E;
	}
}  // namespace hash
