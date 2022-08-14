// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4189)
#endif
#include <fmt/format.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <date/tz.h>
#include <fmt/chrono.h>
#include <charconv>
#include <cov/format.hh>
#include <ctime>
#include <iostream>
#include "../path-utils.hh"

namespace cov {
	namespace parser {
		using placeholder::color;
		using placeholder::format;

#define SIMPLEST_FORMAT(CHAR, RESULT) \
	case CHAR:                        \
		++cur;                        \
		return RESULT
#define SIMPLE_FORMAT(CHAR, RESULT) \
	case CHAR:                      \
		++cur;                      \
		return format { RESULT }
#define DEEPER_FORMAT(CHAR, CB) \
	case CHAR:                  \
		++cur;                  \
		return CB(cur, end)
#define DEEPER_FORMAT1(CHAR, CB, ARG) \
	case CHAR:                        \
		++cur;                        \
		return CB(cur, end, ARG)

		unsigned hex(char c) {
			// there are definitely test going through here, that lend at
			// "return 16", however they do not trigger line counters
			switch (c) {  // GCOV_EXCL_LINE[GCC] -- test-resistant
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					return static_cast<unsigned>(c - '0');
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					return static_cast<unsigned>(c - 'A' + 10);
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					return static_cast<unsigned>(c - 'a' + 10);
			}
			return 16;
		}

		std::optional<unsigned> uint_from(std::string_view view) noexcept {
			unsigned result{};
			if (view.empty()) return std::nullopt;
			auto ptr = view.data();
			auto end = ptr + view.size();
			auto [finish, errc] = std::from_chars(ptr, end, result);
			if (errc == std::errc{} && finish == end) return result;
			return std::nullopt;
		}

		template <typename It>
		std::optional<format> parse_hex(It& cur, It end) {
			if (cur == end) return std::nullopt;
			auto const hi_nybble = hex(*cur);
			if (hi_nybble > 15) return std::nullopt;
			++cur;
			if (cur == end) return std::nullopt;
			auto const lo_nybble = hex(*cur);
			if (lo_nybble > 15) return std::nullopt;
			++cur;

			return format{static_cast<char>(lo_nybble | (hi_nybble << 4))};
		}

		template <typename Item, size_t Length>
		consteval bool is_sorted(Item const (&items)[Length]) {
			if constexpr (Length > 1) {
				for (size_t index = 1; index < Length; ++index) {
					if (items[index] < items[index - 1]) return false;
				}
			}
			return true;
		}

		std::optional<color> find_color(std::string_view name) {
			struct color_pair {
				std::string_view name;
				color value;
				constexpr auto operator<=>(std::string_view other) const {
					return name <=> other;
				}
				constexpr auto operator<=>(color_pair const& other) const =
				    default;
			};
			static_assert(color_pair{"faint blue"sv, color::blue} < "green"sv);
			static_assert("cyan"sv < color_pair{"faint blue"sv, color::cyan});

			static constexpr color_pair colors[] = {
			    {"bg blue"sv, color::bg_blue},
			    {"bg cyan"sv, color::bg_cyan},
			    {"bg green"sv, color::bg_green},
			    {"bg magenta"sv, color::bg_magenta},
			    {"bg rating"sv, color::bg_rating},
			    {"bg red"sv, color::bg_red},
			    {"bg yellow"sv, color::bg_yellow},
			    {"blue"sv, color::blue},
			    {"bold blue"sv, color::bold_blue},
			    {"bold cyan"sv, color::bold_cyan},
			    {"bold green"sv, color::bold_green},
			    {"bold magenta"sv, color::bold_magenta},
			    {"bold normal"sv, color::bold_normal},
			    {"bold rating"sv, color::bold_rating},
			    {"bold red"sv, color::bold_red},
			    {"bold yellow"sv, color::bold_yellow},
			    {"cyan"sv, color::cyan},
			    {"faint blue"sv, color::faint_blue},
			    {"faint cyan"sv, color::faint_cyan},
			    {"faint green"sv, color::faint_green},
			    {"faint magenta"sv, color::faint_magenta},
			    {"faint normal"sv, color::faint_normal},
			    {"faint rating"sv, color::faint_rating},
			    {"faint red"sv, color::faint_red},
			    {"faint yellow"sv, color::faint_yellow},
			    {"green"sv, color::green},
			    {"magenta"sv, color::magenta},
			    {"normal"sv, color::normal},
			    {"rating"sv, color::rating},
			    {"red"sv, color::red},
			    {"reset"sv, color::reset},
			    {"yellow"sv, color::yellow},
			};
			static_assert(is_sorted(colors), "Sort this table!");

			auto const it =
			    std::lower_bound(std::begin(colors), std::end(colors), name);
			if (it != std::end(colors) && it->name == name) return it->value;
			return std::nullopt;
		}

		template <typename It>
		std::optional<format> is_it(std::string_view name,
		                            placeholder::color response,
		                            It& cur,
		                            It end) {
			auto sv_cur = name.begin();
			auto sv_end = name.end();
			while (cur != end && sv_cur != sv_end && *cur == *sv_cur) {
				++cur;
				++sv_cur;
			}
			if (sv_cur == sv_end) return response;
			return std::nullopt;
		}

		template <typename It>
		std::optional<format> parse_color(It& cur, It end) {
			if (cur == end) return std::nullopt;
			if (*cur == '(') {
				++cur;
				if (cur == end) return std::nullopt;
				auto const start = cur;
				while (cur != end && *cur != ')')
					++cur;
				if (cur == end) return std::nullopt;
				auto const color = find_color({start, cur});
				++cur;
				if (color) return *color;
				return std::nullopt;
			}

			if (*cur == 'g') return is_it("green"sv, color::green, cur, end);
			if (*cur == 'b') return is_it("blue"sv, color::blue, cur, end);
			if (*cur == 'r') {
				++cur;
				if (cur == end || *cur != 'e') return std::nullopt;
				++cur;
				if (cur == end) return std::nullopt;
				if (*cur == 'd') {
					++cur;
					return color::red;
				}
				return is_it("set"sv, color::reset, cur, end);
			}

			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse_width(It& cur, It end) {
			static constexpr auto default_total = 76u;
			static constexpr auto default_indent1 = 6u;
			static constexpr auto default_indent2 = 9u;

			if (cur == end || *cur != '(') return std::nullopt;
			++cur;
			auto start = cur;
			while (cur != end && *cur != ')')
				++cur;
			if (cur == end) return std::nullopt;
			auto args = strip(std::string_view{start, cur});
			++cur;
			if (args.empty()) {
				return placeholder::width{default_total, default_indent1,
				                          default_indent2};
			}

			auto comma_pos = args.find(',');
			auto const total = uint_from(strip(args.substr(0, comma_pos)));
			if (!total) return std::nullopt;

			if (comma_pos == std::string_view::npos) {
				return placeholder::width{*total, default_indent1,
				                          default_indent2};
			}
			++comma_pos;
			args = args.substr(comma_pos);

			comma_pos = args.find(',');
			auto const indent1 = uint_from(strip(args.substr(0, comma_pos)));
			if (!indent1) return std::nullopt;

			if (comma_pos == std::string_view::npos) {
				return placeholder::width{*total, *indent1, *indent1};
			}
			++comma_pos;
			args = args.substr(comma_pos);

			comma_pos = args.find(',');
			if (comma_pos != std::string_view::npos) return std::nullopt;
			auto const indent2 = uint_from(strip(args));
			if (!indent2) return std::nullopt;
			if (*indent1 >= *total || *indent2 >= *total) return std::nullopt;
			return placeholder::width{*total, *indent1, *indent2};
		}
		template <typename It>
		std::optional<format> parse_magic(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('d', placeholder::report::magic_ref_names);
				SIMPLE_FORMAT('D',
				              placeholder::report::magic_ref_names_unwrapped);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse_hash(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('r', placeholder::report::hash);
				SIMPLE_FORMAT('c', placeholder::commit::hash);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse_hash_abbr(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('r', placeholder::report::hash_abbr);
				SIMPLE_FORMAT('c', placeholder::commit::hash_abbr);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::person> parse_date(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLEST_FORMAT('d', placeholder::person::date);
				SIMPLEST_FORMAT('r', placeholder::person::date_relative);
				SIMPLEST_FORMAT('t', placeholder::person::date_timestamp);
				SIMPLEST_FORMAT('i', placeholder::person::date_iso_like);
				SIMPLEST_FORMAT('I', placeholder::person::date_iso_strict);
				SIMPLEST_FORMAT('s', placeholder::person::date_short);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse_report(It& cur, It end) {
			using placeholder::person_info;
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('P', placeholder::report::parent_hash);
				SIMPLE_FORMAT('p', placeholder::report::parent_hash_abbr);
				SIMPLE_FORMAT('F', placeholder::report::file_list_hash);
				SIMPLE_FORMAT('f', placeholder::report::file_list_hash_abbr);
				SIMPLE_FORMAT('D', placeholder::report::branch);
				default: {
					auto const person = parse_date(cur, end);
					if (!person) break;
					return format{
					    person_info{placeholder::who::reporter, *person}};
				}
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse_stats(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('P', placeholder::report::lines_percent);
				SIMPLE_FORMAT('T', placeholder::report::lines_total);
				SIMPLE_FORMAT('R', placeholder::report::lines_relevant);
				SIMPLE_FORMAT('C', placeholder::report::lines_covered);
				SIMPLE_FORMAT('r', placeholder::report::lines_rating);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse_person(It& cur,
		                                   It end,
		                                   placeholder::who who) {
			if (cur == end) return std::nullopt;

			using placeholder::person;
			using placeholder::person_info;
			// person_info{who, person::}

			switch (*cur) {
				SIMPLE_FORMAT('n', (person_info{who, person::name}));
				SIMPLE_FORMAT('e', (person_info{who, person::email}));
				SIMPLE_FORMAT('l', (person_info{who, person::email_local}));
				default: {
					auto const person = parse_date(cur, end);
					if (!person) break;
					return format{person_info{who, *person}};
				}
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<format> parse(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('%', '%');
				SIMPLE_FORMAT('n', '\n');
				DEEPER_FORMAT('x', parse_hex);
				DEEPER_FORMAT('C', parse_color);
				DEEPER_FORMAT('w', parse_width);
				DEEPER_FORMAT('m', parse_magic);
				SIMPLE_FORMAT('D', placeholder::report::ref_names_unwrapped);
				SIMPLE_FORMAT('d', placeholder::report::ref_names);
				SIMPLE_FORMAT('s', placeholder::commit::subject);
				SIMPLE_FORMAT('f', placeholder::commit::subject_sanitized);
				SIMPLE_FORMAT('b', placeholder::commit::body);
				SIMPLE_FORMAT('B', placeholder::commit::body_raw);
				DEEPER_FORMAT('H', parse_hash);
				DEEPER_FORMAT('h', parse_hash_abbr);
				DEEPER_FORMAT('r', parse_report);
				DEEPER_FORMAT('p', parse_stats);
				DEEPER_FORMAT1('a', parse_person, placeholder::who::author);
				DEEPER_FORMAT1('c', parse_person, placeholder::who::committer);
			}
			return std::nullopt;
		}
	}  // namespace parser

	formatter formatter::from(std::string_view input) {
		std::vector<placeholder::format> fmts{};
		auto cur = input.begin();
		auto end = input.end();
		auto prev = cur;
		while (cur != end) {
			while (cur != end && *cur != '%')
				++cur;
			if (cur == end) break;
			auto text_end = cur;
			++cur;
			auto arg = parser::parse(cur, end);
			if (!arg) continue;
			if (prev != text_end) fmts.push_back(std::string{prev, text_end});
			fmts.push_back(std::move(*arg));
			prev = cur;
		}
		if (prev != end) fmts.push_back(std::string{prev, end});
		return formatter{std::move(fmts)};
	}
}  // namespace cov
