// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include "cov/hash/hash.hh"

namespace hash {
	struct md5 : basic_hash<md5, 64, 16, endian::little> {
		md5() = default;

		void transform(std::byte const* block) noexcept;
		digest_type on_result() noexcept;

	private:
		uint32_t A_ = 0x67452301L;
		uint32_t B_ = 0xefcdab89L;
		uint32_t C_ = 0x98badcfeL;
		uint32_t D_ = 0x10325476L;
	};
}  // namespace hash
