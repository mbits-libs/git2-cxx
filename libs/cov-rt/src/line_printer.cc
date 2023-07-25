// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <array>
#include <cell/tokens.hh>
#include <cov/app/line_printer.hh>
#include <hilite/hilite.hh>
#include <hilite/none.hh>
#include <variant>

using namespace std::literals;

namespace cov::app::line_printer {
	using namespace lighter;

	enum class mark : unsigned { good, bad, irrelevant };
	using varying_styles = std::array<std::string_view, 3>;
	struct single_color {
		std::string_view value;
	};
	using palette_color = std::variant<single_color, varying_styles>;

	template <typename Value>
	struct value_of {
		using type = Value;
	};
	template <>
	struct value_of<single_color> {
		using type = palette_color;
	};
	template <>
	struct value_of<varying_styles> {
		using type = palette_color;
	};

	struct key_t {
		const std::string_view key;
		template <typename Value>
		using map_entry = cell::map_entry<typename value_of<Value>::type>;
		template <typename Value>
		constexpr map_entry<Value> operator+(
		    Value const& value) const noexcept {
			// GCOV_EXCL_START[Clang] -- ran at compile-time
			return map_entry<Value>{key, value};
		}  // GCOV_EXCL_STOP
	};

	constexpr key_t operator""_k(const char* key, size_t length) noexcept {
		// GCOV_EXCL_START[Clang] -- ran at compile-time
		return key_t{std::string_view{key, length}};
	}  // GCOV_EXCL_STOP

	constexpr single_color operator""_esc(const char* key,
	                                      size_t length) noexcept {
		// GCOV_EXCL_START[Clang] -- ran at compile-time
		return single_color{std::string_view{key, length}};
	}  // GCOV_EXCL_STOP

#if 0
		static constexpr auto reset = "{r}"sv;

		static constinit cell::token_map const palette{
		    "comment"_k + "\033[2;49;92m{//}\033[m"_esc,
		    "meta"_k + "\033[0;49;90m{#}\033[m"_esc,
		    "escape_sequence"_k + "\033[2;49;33m{\\n}\033[m"_esc,
		    "module_name"_k + "\033[2;49;91m{<>}\033[m"_esc,
		    "keyword"_k + "\033[1;37m{kw}\033[m"_esc,
		    "character"_k + "\033[2;49;91m{a}\033[m"_esc,
		    "char_delim"_k + "\033[0;49;91m{\"}\033[m"_esc,
		    "text"_k +
		        varying_styles{
		            "\033[0;49;32m{bad}\033[m"sv,
		            "\033[0;49;31m{good}\033[m"sv,
		            {},
		        },
		};
#else
	static constexpr auto reset = "\033[m"sv;

	static constinit cell::token_map const palette{
	    "comment"_k + "\033[2;49;92m"_esc,
	    "meta"_k + "\033[0;49;90m"_esc,
	    "escape_sequence"_k + "\033[2;49;33m"_esc,
	    "module_name"_k + "\033[2;49;91m"_esc,
	    "keyword"_k +
	        varying_styles{
	            "\033[1;32m"sv,
	            "\033[1;31m"sv,
	            "\033[1;37m"sv,
	        },
	    "character"_k +
	        varying_styles{
	            "\033[2;49;92m"sv,
	            "\033[2;49;91m"sv,
	            "\033[2;49;91m"sv,
	        },
	    "char_delim"_k +
	        varying_styles{
	            "\033[0;49;92m"sv,
	            "\033[0;49;91m"sv,
	            "\033[0;49;91m"sv,
	        },
	    "text"_k +
	        varying_styles{
	            "\033[0;49;32m"sv,
	            "\033[0;49;31m"sv,
	            {},
	        },
	    "fn"_k +
	        varying_styles{
	            "\033[2;49;32m"sv,
	            "\033[2;49;31m"sv,
	            {},
	        },
	};
#endif

	static constinit cell::token_map const aliases{
	    "line_comment"_k + "comment"sv,
	    "block_comment"_k + "comment"sv,
	    "string"_k + "character"sv,
	    "raw_string"_k + "character"sv,
	    "char_encoding"_k + "char_delim"sv,
	    "char_udl"_k + "char_delim"sv,
	    "string_encoding"_k + "char_delim"sv,
	    "string_delim"_k + "char_delim"sv,
	    "string_udl"_k + "char_delim"sv,
	    "universal_character_name"_k + "escape_sequence"sv,
	    "deleted_newline"_k + "escape_sequence"sv,
	    "local_header_name"_k + "module_name"sv,
	    "system_header_name"_k + "module_name"sv,
	    "meta_identifier"_k + "meta"sv,
	    "macro_name"_k + "meta"sv,
	};

	size_t tab_length(std::string_view line, size_t tab_size, size_t column) {
		for (auto c : line) {
			if (c == '\t') {
				column = ((column / tab_size) + 1) * tab_size;
				continue;
			}
			++column;
		}
		return column;
	}

	std::string tab_stopped(std::string_view line,
	                        size_t tab_size,
	                        size_t column = 0) {
		std::string result{};
		result.reserve(tab_length(line, tab_size, column));

		for (auto c : line) {
			if (c == '\t') {
				auto const next_tab_stop = ((column / tab_size) + 1) * tab_size;
				for (auto index = column; index < next_tab_stop; ++index)
					result.push_back(' ');
				column = next_tab_stop;
				continue;
			}
			++column;
			result.push_back(c);
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	struct shell_paint {
		shell_paint(bool use_color) : use_color{use_color} {};

		std::string const& get_color() const noexcept { return color; }

		void set_color(std::string_view next) {
			if ((next.empty() && use_reset) || next == color) return;
			active = false;
			use_reset = next.empty();
			if (!use_reset) color.assign(next);
		}

		void activate(std::string& out) {
			if (active) return;
			active = true;
			if (use_color) out.append(use_reset ? reset : color);
			use_reset = false;
		}

	private:
		bool use_color;
		std::string color{};
		bool active{false};
		bool use_reset{false};
	};

	struct context {
		std::string_view view;
		std::map<std::uint32_t, std::string> const& dict;
		line_printer::mark mark;
		size_t tab_size;
		shell_paint paint;
		size_t column{0};
		std::string result{};

		struct color_stack {
			context* ctx;
			std::string_view shell_color;
			std::string prev;

			explicit color_stack(context* ctx, std::string_view color)
			    : ctx{ctx}, shell_color{color}, prev{ctx->get_color()} {
				if (!shell_color.empty()) ctx->set_color(shell_color);
			}
			~color_stack() { ctx->set_color(prev); }
		};

		std::string_view find_color(unsigned tok) const noexcept {
			auto const it = dict.find(tok);
			auto token =
			    it != dict.end() ? it->second : hl::none::token_to_string(tok);
			return find_color(token);
		}

		std::string_view find_color(std::string_view token) const noexcept {
			if (token.empty()) return {};
			auto const alias = aliases.find(token);
			if (alias != aliases.end()) token = alias->value;
			auto clr = palette.find(token);
			if (clr == palette.end()) clr = palette.find("text"sv);
			if (clr == palette.end()) return {};
			return std::visit(
			    [mark = mark](auto const& style) {
				    if constexpr (std::same_as<decltype(style),
				                               single_color const&>)
					    return style.value;
				    else
					    return style[std::to_underlying(mark)];
			    },
			    clr->value);
		}

		std::string const& get_color() const noexcept {
			return paint.get_color();
		}
		void set_color(std::string_view value) { paint.set_color(value); }
		void paint_color() { paint.activate(result); }

		void add_text(text_span const& text) {
			paint_color();
			result.append(
			    tab_stopped(view.substr(text.begin, text.end - text.begin),
			                tab_size, column));
		}

		void add_span(span const& S) {
			color_stack select{this, find_color(S.kind)};
			visit_span(S.contents);
		}

		void visit_span(highlighted_line const& items) {
			struct visitor {
				context* self;
				void operator()(text_span const& text) { self->add_text(text); }
				void operator()(span const& S) { self->add_span(S); }
			};
			for (auto const& item : items)
				std::visit(visitor{this}, item.base());
		}
	};

	std::string to_string(std::optional<unsigned> const& count,
	                      std::string_view view,
	                      highlighted_line const& items,
	                      std::map<std::uint32_t, std::string> const& dict,
	                      bool use_color,
	                      size_t tab_size) {
		auto mark = !count ? mark::irrelevant : *count ? mark::good : mark::bad;
		auto line_mark = !count ? ' ' : *count ? '+' : '-';
		context ctx{view, dict, mark, tab_size, {use_color}};

		{
			context::color_stack paint{&ctx, ctx.find_color("text"sv)};
			ctx.paint_color();
			ctx.result.push_back(line_mark);
			ctx.visit_span(items);
		}
		ctx.paint_color();  // EOL reset

		return ctx.result;
	}

	std::string to_string(std::optional<unsigned> const& count,
	                      std::string_view view,
	                      bool shortened,
	                      bool use_color,
	                      size_t tab_size) {
		auto mark = !count ? mark::irrelevant : *count ? mark::good : mark::bad;

		std::map<std::uint32_t, std::string> empty{};
		context ctx{view, empty, mark, tab_size, {use_color}};

		{
			context::color_stack paint{&ctx, ctx.find_color("fn"sv)};
			ctx.paint_color();
			ctx.result.append(view);
		}
		ctx.paint_color();  // EOL reset
		if (shortened) {
			// LCOV_EXCL_START -- TODO: Add pty to test_driver
			if (use_color) ctx.result.append("\033[2;49;39m"sv);
			ctx.result.append("..."sv);
			if (use_color) ctx.result.append("\033[m"sv);
			// GCOV_EXCL_STOP
		}

		return ctx.result;
	}
}  // namespace cov::app::line_printer
