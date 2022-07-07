// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <hilite/highlighter.hh>
#include <hilite/hilite.hh>
#include <iostream>
#include <string_view>

namespace highlighter {
	struct printer_state {
		std::string_view hl_macro{};
		std::string_view span_prefix{};
		std::map<unsigned, std::string> const* dict;
		static printer_state& get() {
			static printer_state instance{};
			return instance;
		}

		static void setup(std::string_view hl_macro,
		                  std::string_view span_prefix) {
			auto& inst = get();
			inst.hl_macro = hl_macro;
			inst.span_prefix = span_prefix;
			inst.dict = nullptr;
		}
	};
	struct visitor {
		std::ostream& out;

		static std::string_view hl_enum(unsigned tok) {
			using namespace std::literals;
#define TOKEN_NAME(x) \
	case hl::x:       \
		return #x##sv;
			switch (tok) { HILITE_TOKENS(TOKEN_NAME) }
			return {};
#undef TOKEN_NAME
		}

		inline void operator()(text_span const& text);
		inline void operator()(span const& s);
	};

	struct print_to {
		highlighted_line const& items;

		friend std::ostream& operator<<(std::ostream& out, print_to const& p) {
			bool first = true;
			for (auto const& item : p.items) {
				if (first)
					first = false;
				else
					out << ", ";
				std::visit(visitor{out}, item.base());
			}

			return out;
		}
	};

	inline void visitor::operator()(text_span const& text) {
		out << "text_span{" << text.begin << "u," << text.end << "u}";
	}
	inline void visitor::operator()(span const& s) {
		auto name = hl_enum(s.kind);
		auto const span_prefix = printer_state::get().span_prefix;
		if (!span_prefix.empty()) {
			auto const dict = printer_state::get().dict;
			if (name.empty() && dict) {
				auto it = dict->find(s.kind);
				if (it != dict->end()) name = it->second;
			}
			if (!name.empty()) {
				out << "tok_" << span_prefix << '_' << name << '{'
				    << print_to{s.contents} << '}';
				return;
			}
			name = hl_enum(s.kind);
		}

		out << "span{";
		if (name.empty())
			out << s.kind << 'u';
		else
			out << "hl::" << name;
		out << ", {" << print_to{s.contents} << "}}";
	}

	inline void PrintTo(highlighted_line const& line, std::ostream* os) {
		*os << print_to{line};
	}

	inline void PrintTo(span_node const& node, std::ostream* os) {
		std::visit(visitor{*os}, node.base());
	}

	inline void PrintTo(highlights::line const& line, std::ostream* os) {
		*os << "hl_line{" << line.start << "u, {" << print_to{line.contents}
		    << '}';
		if (line.contents.size() > 1) *os << ',';
		*os << '}';
	}

	inline void PrintTo(highlights const& hl, std::ostream* os) {
		auto* const state = &printer_state::get();
		auto const hl_macro = state->hl_macro;
		*os << "{ .dict{";
		bool first = true;
		for (auto const& [key, value] : hl.dict) {
			if (first)
				first = false;
			else
				*os << ',';

			if (hl_macro.empty()) {
				*os << "{" << key << "u,\"" << value << "\"}";
				continue;
			}

			*os << hl_macro << '(' << value << ')';
		}
		*os << "}, .lines{";
		for (auto const& line : hl.lines) {
			PrintTo(line, os);
			*os << ',';
		}
		*os << "},}";
	}
}  // namespace highlighter

namespace highlighter::testing {
	using namespace std::literals;

	template <unsigned tok>
	struct tok_ : span {
		template <typename... Node>
		tok_(Node const&... nodes) : span{tok, {as_node(nodes)...}} {}

		static span_node as_node(text_span const& span) { return span; }
		static span_node as_node(span const& span) { return span; }
	};

	inline std::ostream& single_char(std::ostream& out, char c) {
		switch (c) {
			case '"':
				return out << "\\\"";
			case '\n':
				return out << "\\n";
			case '\r':
				return out << "\\r";
			case '\t':
				return out << "\\t";
			case '\v':
				return out << "\\v";
			case '\a':
				return out << "\\a";
			default:
				if (!std::isprint(static_cast<unsigned char>(c))) {
					char buffer[10];
					sprintf(buffer, "\\x%02x", c);
					return out << buffer;
				} else
					return out << c;
		}
	}

	inline std::ostream& print_view(std::ostream& out, std::string_view text) {
		out << '"';
		for (auto c : text) {
			single_char(out, c);
		}
		return out << '"';
	}

	struct print {
		highlighted_line const& items;
		std::string_view view;
		std::string_view& curr_color;
		std::map<std::uint32_t, std::string> const& dict;

		struct visitor {
			std::ostream& out;
			std::string_view view;
			std::string_view& curr_color;
			std::map<std::uint32_t, std::string> const& dict;

			static std::string_view hl_color(unsigned tok) {
#define TOKEN_NAME(x) \
	case hl::x:       \
		return #x##sv;
				switch (static_cast<hl::token>(tok)) {
					HILITE_TOKENS(TOKEN_NAME)
				}
				return {};
#undef TOKEN_NAME
			}

			std::string_view color(unsigned tok) {
				switch (tok) {
					case hl::line_comment:
					case hl::block_comment:
						return "\033[2;49;92m"sv;
					case hl::meta:
						return "\033[2;49;90m"sv;
					case hl::keyword:
						return "\033[0;49;34m"sv;
					case hl::character:
					case hl::string:
					case hl::raw_string:
						return "\033[2;49;91m"sv;
					case hl::char_encoding:
					case hl::char_delim:
					case hl::char_udl:
					case hl::string_encoding:
					case hl::string_delim:
					case hl::string_udl:
						return "\033[0;49;91m"sv;
					case hl::escape_sequence:
						return "\033[1;49;93m"sv;
					case hl::module_name:
						return "\033[2;49;91m"sv;
					default: {
						auto const it = dict.find(tok);
						if (it != dict.end()) {
							if (it->second == "local_header_name"sv ||
							    it->second == "system_header_name"sv) {
								return "\033[2;49;91m"sv;
							}
							if (it->second == "macro_name"sv) {
								return "\033[1;49;90m"sv;
							}
							if (it->second == "universal_character_name"sv) {
								return "\033[1;49;93m"sv;
							}
						}
					}
				}
				return {};
			}

			void operator()(text_span const& text) {
				out << view.substr(text.begin, text.end - text.begin);
			}
			void operator()(span const& s) {
				static constexpr auto reset = "\033[m"sv;
				auto shell_color = color(s.kind);

				auto const prev = curr_color;
				if (!shell_color.empty()) {
					curr_color = shell_color;
					out << curr_color;
				}

				out << print{s.contents, view, curr_color, dict};

				if (!shell_color.empty()) {
					curr_color = prev;
					out << reset << curr_color;
				}
			}
		};

		friend std::ostream& operator<<(std::ostream& out, print const& p) {
			visitor v{out, p.view, p.curr_color, p.dict};
			for (auto const& item : p.items)
				std::visit(v, item.base());

			return out;
		}
	};

	inline size_t length_of(highlighted_line const& items) {
		if (items.empty()) return 0u;
		return items.back().furthest_end();
	}

	using hl_line = highlights::line;
}  // namespace highlighter::testing
