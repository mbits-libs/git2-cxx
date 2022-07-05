// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace highlighter {
	struct text_span {
		std::size_t begin;
		std::size_t end;
		auto operator<=>(text_span const&) const noexcept = default;
	};

	struct span_node;
	struct span {
		using contents_type = std::vector<span_node>;
		unsigned kind{};
		contents_type contents{};

		inline bool operator==(span const&) const noexcept;
		inline std::size_t furthest_end() const noexcept;
		static inline void cleanup(contents_type& items);
		void cleanup() { cleanup(contents); }
	};

	using highlighted_line = span::contents_type;

	struct span_node : std::variant<text_span, span> {
		using base_type = std::variant<text_span, span>;
		using base_type::variant;
		base_type& base() noexcept { return *this; }
		base_type const& base() const noexcept { return *this; }
		bool operator==(span_node const& other) const noexcept {
			if (index() != other.index()) return false;
			if (!index())
				return std::get<0>(base()) == std::get<0>(other.base());
			return std::get<1>(base()) == std::get<1>(other.base());
		}
		size_t furthest_end() const noexcept {
			struct visitor {
				size_t operator()(text_span const& text) const noexcept {
					return text.end;
				}
				size_t operator()(span const& s) const noexcept {
					return s.furthest_end();
				}
			};

			return std::visit(visitor{}, base());
		}
		bool valid() const noexcept {
			struct visitor {
				bool operator()(text_span const&) const noexcept {
					return true;
				}
				bool operator()(span const& s) const noexcept {
					return !s.contents.empty();
				}
			};

			return std::visit(visitor{}, base());
		}
		void cleanup() {
			struct visitor {
				void operator()(text_span const&) const noexcept {}
				void operator()(span& s) const noexcept { s.cleanup(); }
			};

			return std::visit(visitor{}, base());
		}
	};

	inline bool span::operator==(span const& other) const noexcept = default;

	inline size_t span::furthest_end() const noexcept {
		if (contents.empty()) return 0u;
		return contents.back().furthest_end();
	}

	inline void span::cleanup(contents_type& items) {
		for (auto& item : items)
			item.cleanup();
		auto it =
		    std::remove_if(items.begin(), items.end(),
		                   [](auto const& item) { return !item.valid(); });
		items.erase(it, items.end());
	}

	struct highlights {
		struct line {
			std::size_t start;
			highlighted_line contents;
			bool operator==(line const&) const noexcept = default;
		};

		std::map<unsigned, std::string> dict;
		std::vector<line> lines;
		bool operator==(highlights const&) const noexcept = default;

		static highlights from(std::string_view contents,
		                       std::string_view as_filename);
	};
}  // namespace highlighter
