// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#define HILITE_TOKENS(X) \
	X(whitespace)        \
	X(newline)           \
	X(line_comment)      \
	X(block_comment)     \
	X(identifier)        \
	X(keyword)           \
	X(module_name)       \
	X(known_ident_1)     \
	X(known_ident_2)     \
	X(known_ident_3)     \
	X(punctuator)        \
	X(number)            \
	X(character)         \
	X(char_encoding)     \
	X(char_delim)        \
	X(char_udl)          \
	X(string)            \
	X(string_encoding)   \
	X(string_delim)      \
	X(string_udl)        \
	X(escape_sequence)   \
	X(raw_string)        \
	X(meta)              \
	X(meta_identifier)

namespace hl {
	enum token : unsigned {
#define LIST_TOKENS(x) x,
		HILITE_TOKENS(LIST_TOKENS)
#undef LIST_TOKENS
	};

	struct token_t {
		size_t start;
		size_t stop;
		hl::token kind;
		auto cmp(const token_t& rhs) const {
			if (auto const cmp = start <=> rhs.start; cmp != 0) return cmp;
			if (auto const cmp = rhs.stop <=> stop; cmp != 0) return cmp;
			return kind <=> rhs.kind;
		}

		// when sorting, by default don't look at the numerical
		// values of token kind, as this holds no merit.
		auto operator<=>(const token_t& rhs) const noexcept {
			if (start == rhs.start) return rhs.stop <=> stop;
			return start <=> rhs.start;
		}

		bool operator==(const token_t& rhs) const {
			return (start == rhs.start) && (stop == rhs.stop) &&
			       (kind == rhs.kind);
		}
	};
	using tokens = std::vector<token_t>;

	struct token_stack_t {
		size_t end;
		hl::token kind;
	};

	struct callback {
		callback();
		callback(const callback&) = delete;
		callback(callback&&) = delete;
		callback& operator=(const callback&) = delete;
		callback& operator=(callback&&) = delete;

		virtual ~callback();
		virtual void on_line(std::size_t start,
		                     std::size_t length,
		                     const tokens& highlights) = 0;
	};
}  // namespace hl
