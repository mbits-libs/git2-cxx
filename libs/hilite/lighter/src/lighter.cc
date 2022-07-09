// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <hilite/lighter.hh>

#include <algorithm>
#include <cell/tokens.hh>
#include <hilite/hilite.hh>
#include <hilite/none.hh>
#include <stack>
#include <unordered_set>

#define HAS_ANY_HILITE 0

#ifdef __has_include
#if __has_include("hilite/syntax/cxx.hh")
#include "hilite/syntax/cxx.hh"
#define HAS_CXX 1
#undef HAS_ANY_HILITE
#define HAS_ANY_HILITE 1
#endif  // __has_include("hilite/syntax/cxx.hh")
#if __has_include("hilite/syntax/py3.hh")
#include "hilite/syntax/py3.hh"
#define HAS_PY3 1
#undef HAS_ANY_HILITE
#define HAS_ANY_HILITE 1
#endif  // __has_include("hilite/syntax/py3.hh")
#endif

namespace lighter {
	namespace {
#define LIST_TOKENS(x) hl::x,
		constexpr auto max_token_kind = std::max({HILITE_TOKENS(LIST_TOKENS)});
#undef LIST_TOKENS
		struct hl_iface {
			void (*tokenize)(std::string_view const&, hl::callback&);
			std::string_view (*to_string)(unsigned) noexcept;
		};

		struct key_t {
			const std::string_view value;
			template <typename Value>
			friend constexpr cell::map_entry<Value> operator+(
			    key_t const& key,
			    Value const& value) noexcept {
				return cell::map_entry<Value>(key.value, value);
			}
		};

		constexpr key_t operator""_k(const char* key, size_t length) noexcept {
			return key_t{std::string_view{key, length}};
		}

		// GCOV_EXCL_START
		std::string_view none_to_string(unsigned) noexcept { return {}; }
		// GCOV_EXCL_STOP

#ifdef HAS_CXX
		constexpr hl_iface cxx{hl::cxx::tokenize, hl::cxx::token_to_string};
#endif
#ifdef HAS_PY3
		constexpr hl_iface python{hl::py3::tokenize, hl::py3::token_to_string};
#endif
		constexpr hl_iface none{hl::none::tokenize, none_to_string};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4307)  // C4307: '*': integral constant overflow
		// The overflow is inside cell::fnv::ce_1a_hash and is expected there
		// but the warning is reported on every constexpr token_set or token_map
#endif
#if HAS_ANY_HILITE
		constexpr cell::token_map extensions{
#ifdef HAS_CXX
		    // C
		    "c"_k + cxx,

		    // C++
		    "C"_k + cxx,
		    "c++"_k + cxx,
		    "cc"_k + cxx,
		    "cpp"_k + cxx,
		    "cxx"_k + cxx,
		    "h"_k + cxx,
		    "hh"_k + cxx,
		    "h++"_k + cxx,
		    "hpp"_k + cxx,
		    "hxx"_k + cxx,
		    "in"_k + cxx,
		    "txx"_k + cxx,

		    // Arduino C++
		    "ino"_k + cxx,

		    // Objective-C
		    "m"_k + cxx,

		    // Objective-C++
		    "M"_k + cxx,
		    "mm"_k + cxx,
		    "hm"_k + cxx,

		    // C for CUDA
		    "cu"_k + cxx,
#endif  // defined(HAS_CXX)
#ifdef HAS_PY3
		    // Python
		    "py"_k + python,
		    "pyw"_k + python,

		    // Py3 (used rarely)
		    "py3"_k + python,

		    // Stub file
		    "pyi"_k + python,
#endif  // defined(HAS_PY3)
		};
#endif  // HAS_ANY_HILITE
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		hl_iface const& locate_lighter(
		    [[maybe_unused]] std::string_view as_filename) {
#if HAS_ANY_HILITE
			auto slash = as_filename.rfind('/');
			if (slash != std::string_view::npos)
				++slash;
			else
				slash = 0;

			auto const sub = as_filename.substr(slash);
			auto dot = sub.rfind('.');

			if (dot != std::string_view::npos && dot != 0) {
				if (dot) {
					auto it = extensions.find(sub.substr(dot + 1));
					if (it != extensions.end()) return it->value;
				}
			}
#endif  // HAS_ANY_HILITE

			return none;
		}

		struct node {
			hl::token_t tok{};
			std::vector<node> children{};

			highlighted_line spans() const {
#if 0
				// enable this block to see the marker in the oom.node_spans
				// test
				struct enter_leave {
					enter_leave() { fputs("node::spans :: ENTER\n", stdout); }
					~enter_leave() { fputs("node::spans :: LEAVE\n", stdout); }
				};
				enter_leave marker{};
#endif
				highlighted_line result{};
				auto prev = tok.start;
				for (auto const& child : children) {
					if (prev < child.tok.start)
						result.push_back(text_span{
						    .begin = static_cast<std::uint32_t>(prev),
						    .end = static_cast<std::uint32_t>(child.tok.start),
						});

					result.push_back(span{
					    .kind = static_cast<std::uint32_t>(child.tok.kind),
					    .contents = child.spans(),
					});
					prev = child.tok.stop;
				}

				if (prev < tok.stop) {
					result.push_back(text_span{
					    .begin = static_cast<std::uint32_t>(prev),
					    .end = static_cast<std::uint32_t>(tok.stop),
					});
				}

				span::cleanup(result);
				return result;
			}

			static node from(hl::tokens const& tokens, size_t length) {
				std::stack<node> nodes{};
				nodes.push({{
				    .start = 0,
				    .stop = length,
				    .kind = hl::whitespace,
				}});

				for (auto const& tok : tokens) {
					if (tok.kind == hl::whitespace) continue;
					while (!nodes.empty() && nodes.top().tok.stop < tok.stop) {
						auto top = nodes.top();
						nodes.pop();
						if (nodes.empty()) break;
						nodes.top().children.push_back(std::move(top));
					}

					nodes.push({tok});
				}

				node result{};
				while (!nodes.empty()) {
					result = nodes.top();
					nodes.pop();
					if (!nodes.empty())
						nodes.top().children.push_back(std::move(result));
				}

				return result;
			}
		};

		highlights tokenize(std::string_view contents, hl_iface const& iface) {
			struct callback : hl::callback {
				highlights& result;
				std::unordered_set<unsigned> dict{};
				callback(highlights& result) : result{result} {}
				void on_line(std::size_t start,
				             std::size_t length,
				             hl::tokens const& tokens) final {
					result.lines.push_back({
					    .start = start,
					    .contents = node::from(tokens, length).spans(),
					});

					for (auto const& tok : tokens) {
						if (tok.kind > max_token_kind) dict.insert(tok.kind);
					}
				}
			};

			highlights result{};
			callback returned{result};
			iface.tokenize(contents, returned);
			for (auto kind : returned.dict) {
				auto name = iface.to_string(kind);
				if (name.empty()) continue;

				result.dict[kind] = name;
			}
			return result;
		}
	}  // namespace

	highlights highlights::from(std::string_view contents,
	                            std::string_view as_filename) {
		return tokenize(contents, locate_lighter(as_filename));
	}
}  // namespace lighter
