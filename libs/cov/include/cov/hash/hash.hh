// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cstring>
#include <string>

#include <git2/bytes.hh>

namespace hash {
	enum class endian {
#ifdef _WIN32
		little = 0,
		big = 1,
		native = little
#else
		little = __ORDER_LITTLE_ENDIAN__,
		big = __ORDER_BIG_ENDIAN__,
		native = __BYTE_ORDER__
#endif
	};

	template <endian e>
	struct host;

	template <>
	struct host<endian::little> {
		static inline uint32_t c2l(const std::byte*& b) noexcept {
			auto l = uint32_t{};
			l |= (std::to_integer<uint32_t>(*b++));
			l |= (std::to_integer<uint32_t>(*b++) << 8);
			l |= (std::to_integer<uint32_t>(*b++) << 16);
			l |= (std::to_integer<uint32_t>(*b++) << 24);
			return l;
		}

		static inline void l2c(uint32_t l, std::byte*& b) noexcept {
			*b++ = std::byte{
			    static_cast<std::underlying_type_t<std::byte>>(l & 0xFF)};
			*b++ = std::byte{static_cast<std::underlying_type_t<std::byte>>(
			    (l >> 8) & 0xFF)};
			*b++ = std::byte{static_cast<std::underlying_type_t<std::byte>>(
			    (l >> 16) & 0xFF)};
			*b++ = std::byte{static_cast<std::underlying_type_t<std::byte>>(
			    (l >> 24) & 0xFF)};
		}

		static inline void ll2c(uint32_t lo,
		                        uint32_t hi,
		                        std::byte*& b) noexcept {
			l2c(lo, b);
			l2c(hi, b);
		}
	};
	template <>
	struct host<endian::big> {
		static inline uint32_t c2l(const std::byte*& b) noexcept {
			auto l = uint32_t{};
			l |= (std::to_integer<uint32_t>(*b++) << 24);
			l |= (std::to_integer<uint32_t>(*b++) << 16);
			l |= (std::to_integer<uint32_t>(*b++) << 8);
			l |= (std::to_integer<uint32_t>(*b++));
			return l;
		}

		static inline void l2c(uint32_t l, std::byte*& b) noexcept {
			*b++ = std::byte{static_cast<std::underlying_type_t<std::byte>>(
			    (l >> 24) & 0xFF)};
			*b++ = std::byte{static_cast<std::underlying_type_t<std::byte>>(
			    (l >> 16) & 0xFF)};
			*b++ = std::byte{static_cast<std::underlying_type_t<std::byte>>(
			    (l >> 8) & 0xFF)};
			*b++ = std::byte{
			    static_cast<std::underlying_type_t<std::byte>>(l & 0xFF)};
		}

		static inline void ll2c(uint32_t lo,
		                        uint32_t hi,
		                        std::byte*& b) noexcept {
			l2c(hi, b);
			l2c(lo, b);
		}
	};

	template <size_t length>
	struct digest_t {
		static constexpr size_t digest_length = length;

		std::byte data[length];
		std::string str() const {
			static constexpr char alphabet[] = "0123456789abcdef";
			std::string out;
			out.reserve(length * 2);
			for (auto b : data) {
				auto const c = std::to_integer<unsigned char>(b);
				out.push_back(alphabet[(c >> 4) & 0xf]);
				out.push_back(alphabet[c & 0xf]);
			}
			return out;
		}
	};

	template <typename Derived,
	          size_t BlockByteSize,
	          size_t DigestByteSize,
	          endian DataOrder>
	struct basic_hash {
		static_assert(
		    !(BlockByteSize % 4),
		    "BlockByteSize must be able to hold whole uint32_t values");

		static constexpr auto block_byte_size = BlockByteSize;
		static constexpr auto block_without_count = BlockByteSize - 8;
		static constexpr auto digest_byte_size = DigestByteSize;

		using digest_type = digest_t<digest_byte_size>;

		basic_hash() = default;

		Derived& update(git::bytes const& data) noexcept {
			auto it = data.begin();
			auto length = data.size();

			auto const lo = count_lo_ + (static_cast<uint32_t>(length) << 3);
			if (lo < count_lo_) ++count_hi_;
			count_hi_ += static_cast<uint32_t>(length >> 29);
			count_lo_ = lo;

			auto const rest = block_byte_size - span_used_;
			if (length < rest) {
				std::memcpy(span_ + span_used_, it, length);
				span_used_ += length;
				return *static_cast<Derived*>(this);
			}

			if (span_used_) {
				std::memcpy(span_ + span_used_, it, rest);
				span_used_ = 0;

				std::advance(it, rest);
				length -= rest;

				static_cast<Derived*>(this)->transform(span_);
			}

			while (length >= block_byte_size) {
				static_cast<Derived*>(this)->transform(it);
				std::advance(it, block_byte_size);
				length -= block_byte_size;
			}

			if (length) {
				span_used_ = length;
				std::memcpy(span_, it, length);
			}

			return *static_cast<Derived*>(this);
		}

		digest_type finalize() noexcept {
			// span_used_ is always less than block_byte_size
			span_[span_used_++] = std::byte{0x80};

			if (span_used_ > block_without_count) {
				memset(span_ + span_used_, 0, block_byte_size - span_used_);
				span_used_ = 0;

				static_cast<Derived*>(this)->transform(span_);
			}

			memset(span_ + span_used_, 0, block_without_count - span_used_);
			span_used_ = 0;

			auto* it = span_ + block_without_count;
			ll2c(count_lo_, count_hi_, it);

			static_cast<Derived*>(this)->transform(span_);
			count_lo_ = 0;
			count_hi_ = 0;

			return static_cast<Derived*>(this)->on_result();
		}

		static digest_type once(git::bytes const& data) {
			return Derived{}.update(data).finalize();
		}

	protected:
		static inline uint32_t c2l(const std::byte*& b) noexcept {
			return host<DataOrder>::c2l(b);
		}

		static inline void l2c(uint32_t l, std::byte*& b) noexcept {
			return host<DataOrder>::l2c(l, b);
		}

		static inline void ll2c(uint32_t lo,
		                        uint32_t hi,
		                        std::byte*& b) noexcept {
			return host<DataOrder>::ll2c(lo, hi, b);
		}

		uint32_t count_lo_{};
		uint32_t count_hi_{};
		std::byte span_[block_byte_size];
		size_t span_used_{};
	};
}  // namespace hash
