// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include "cov/hash/hash.hh"

namespace hash {
	struct sha1 : basic_hash<sha1, 64, 20, endian::big> {
		sha1() = default;

		void transform(std::byte const* block) noexcept;
		digest_type on_result() noexcept;

	private:
		uint32_t A_ = 0x67452301L;
		uint32_t B_ = 0xEFCDAB89L;
		uint32_t C_ = 0x98BADCFEL;
		uint32_t D_ = 0x10325476L;
		uint32_t E_ = 0xC3D2E1F0L;
	};
}  // namespace hash
