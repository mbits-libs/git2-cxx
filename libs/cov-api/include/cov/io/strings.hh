// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/io/types.hh>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cov {
	struct read_stream;
}

namespace cov::io {
	class strings_view_base {
	public:
		constexpr strings_view_base() = default;
		constexpr strings_view_base(strings_view_base const&) = default;
		constexpr strings_view_base(strings_view_base&&) = default;
		constexpr strings_view_base& operator=(strings_view_base const&) =
		    default;
		constexpr strings_view_base& operator=(strings_view_base&&) = default;

		class const_iterator {
			friend class strings_view_base;
			char const* ptr_{};
			constexpr const_iterator(char const* ptr) : ptr_{ptr} {};

		public:
			constexpr const_iterator() = default;
			constexpr const_iterator(const_iterator const&) = default;
			constexpr const_iterator(const_iterator&&) = default;
			constexpr const_iterator& operator=(const_iterator const&) =
			    default;
			constexpr const_iterator& operator=(const_iterator&&) = default;

			constexpr bool operator==(
			    const_iterator const& rhs) const noexcept {
				return ptr_ == rhs.ptr_;
			}

			constexpr const_iterator& operator++() noexcept {
				auto const off =
				    ptr_ ? std::string::traits_type::length(ptr_) + 1 : 0;
				ptr_ += off;
				return *this;
			}

			constexpr std::string_view operator*() const noexcept {
				return ptr_ ? std::string_view{ptr_,
				                               std::string::traits_type::length(
				                                   ptr_)}
				            : std::string_view{};
			}
		};

		constexpr const_iterator begin() const noexcept { return block_; }

		constexpr const_iterator end() const noexcept { return block_ + size_; }

		constexpr char const* data() const noexcept { return block_; }
		constexpr size_t size() const noexcept { return size_; }
		constexpr std::string_view at(size_t offset) const noexcept {
			return offset >= size_ ? std::string_view{} : block_ + offset;
		}
		constexpr std::string_view at(io::str offset) const noexcept {
			return at(std::to_underlying(offset));
		}

		constexpr bool is_valid(size_t offset) const noexcept {
			return (offset < size_) && (!offset || !block_[offset - 1]);
		}

		constexpr bool is_valid(io::str offset) const noexcept {
			return is_valid(std::to_underlying(offset));
		}

		template <typename Header>
		constexpr block after() const noexcept {
			return {.offset = static_cast<uint32_t>(sizeof(Header) /
			                                        sizeof(uint32_t)),
			        .size = static_cast<uint32_t>(size() / sizeof(uint32_t))};
		}
		template <typename Header, typename Entry>
		constexpr array_ref align_array(size_t count) const noexcept {
			return array_ref::from<Entry>(size() + sizeof(Header), count);
		}

	protected:
		constexpr void update(char const* block, size_t size) noexcept {
			block_ = block;
			size_ = size;
		}

	private:
		char const* block_{};
		size_t size_{};
	};

	struct strings_view : strings_view_base {
		std::vector<char> data;

		strings_view() = default;
		strings_view(strings_view const&);
		strings_view(strings_view&&) noexcept;
		strings_view& operator=(strings_view const&);
		strings_view& operator=(strings_view&&) noexcept;

		bool load_from(cov::read_stream& in, block const& strings);
		void resync() noexcept;
	};

	class strings_block : public strings_view_base {
		std::vector<size_t> offsets_{};
		std::vector<char> data_{};
		inline void update_() noexcept;

	public:
		using strings_view_base::is_valid;
		using strings_view_base::strings_view_base;

		strings_block(std::vector<char> data) : data_{std::move(data)} {}

		size_t data_size() const noexcept { return data_.size(); }
		bool reserve_offsets(size_t length) noexcept;
		bool reserve_data(size_t length) noexcept;
		bool append(std::string_view value) noexcept;
		bool fill32() noexcept;
		bool is_valid() const noexcept;
		size_t locate(std::string_view str) const noexcept;
		size_t locate_or(std::string_view str,
		                 size_t value_if_invalid) const noexcept;
	};

	class strings_builder {
		std::set<std::string, std::less<>> strings_{};

	public:
		strings_builder& insert(std::string_view) noexcept;
		strings_block build() const noexcept;
	};
}  // namespace cov::io
