// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

// https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace cxx_filt {
	template <typename Item>
	struct vector_view {
		Item* data_;
		size_t size_;
		template <typename V>
		vector_view(std::vector<V> const& v)
		    : data_{v.data()}, size_{v.size()} {}
		vector_view(vector_view const& v) : data_{v.data_}, size_{v.size_} {}
		vector_view(Item* data, size_t size) : data_{data}, size_{size} {}

		vector_view sub_view(size_t start, size_t count = std::string::npos) {
			start = std::min(start, size_);
			count = std::min(count, size_ - start);
			return {data_ + start, count};
		}

		auto begin() const noexcept { return data_; }
		auto end() const noexcept { return data_ + size_; }
		auto& operator[](size_t index) const noexcept { return data_[index]; }
		auto size() const noexcept { return size_; }
		auto data() const noexcept { return data_; }
	};

	struct Reference {
		std::string id;
	};

	enum class Token {
		NONE,
		SPACE,
		COMMA,
		COLON,
		HASH,
		SCOPE,
		PTR,
		REF,
		REF_REF,
		CONST,
		VOLATILE,
		OPEN_ANGLE_BRACKET,
		CLOSE_ANGLE_BRACKET,
		OPEN_PAREN,
		CLOSE_PAREN,
		OPEN_CURLY,
		CLOSE_CURLY,
		OPEN_BRACKET,
		CLOSE_BRACKET,
	};

	std::string_view str_from_token(Token tok);
	std::string_view dbg_from_token(Token tok);
}  // namespace cxx_filt
