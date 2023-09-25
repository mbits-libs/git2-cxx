// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "excludes.hh"
#include <fmt/format.h>
#include <algorithm>
#include <ctre.hpp>
#include <native/str.hh>

using namespace std::literals;

namespace cov::app::strip {
	static constexpr auto matches_tags =
	    ctre::search<"(?:G|L|GR)COV_EXCL_(?:START|LINE)\\[([^\\]]+)\\]">;
	static constexpr auto matches_line =
	    ctre::search<"(?:G|L|GR)COV_EXCL_LINE">;
	static constexpr auto matches_start =
	    ctre::search<"(?:G|L|GR)COV_EXCL_START">;
	static constexpr auto matches_stop =
	    ctre::search<"(?:G|L|GR)COV_EXCL_STOP">;
	static constexpr auto matches_end = ctre::search<"(?:G|L|GR)COV_EXCL_END">;

	bool excludes::has_any_marker(std::string_view markers) {
		bool has = false;
		split(markers, ',', [&](unsigned, std::string_view marker) {
			if (has) return;
			auto const key = tolower(trim(marker));
			for (auto const& known : valid_markers) {
				if (known == key) {
					has = true;
					break;
				}
			}
		});
		return has;
	}

	template <typename Match>
	unsigned column_from(std::string_view text, Match const& matcher) {
		auto const group0 = matcher.template get<0>().to_view();
		auto const diff = group0.data() - text.data();
		decltype(diff) zero = 0;
		return static_cast<unsigned>(std::max(zero, diff) + 1);
	}

	void excludes::on_line(unsigned line_no, std::string_view text) {
		if (trim(text).empty()) {
			empties.insert(line_no);
			return;
		}

		if (auto const has_tags = matches_tags(text); has_tags) {
			auto tags = has_tags.get<1>().to_view();
			if (!has_any_marker(tags)) return;
		}

		auto switch_off = false;
		if (matches_stop(text)) {
			switch_off = true;
		} else if (auto end_match = matches_end(text); end_match) {
			if (inside_exclude) {
				auto const end = end_match.get<0>().to_view();
				warn(line_no, column_from(text, end_match),
				     fmt::format("found {}; did you mean {}?"sv, end,
				                 stop_for(end, "_END"sv)));
			}
		} else if (auto start_match = matches_start(text); start_match) {
			if (inside_exclude) {
				warn(line_no, column_from(text, start_match),
				     fmt::format("double start: found {}"sv,
				                 start_match.get<0>().to_view()));
				note(std::get<0>(last_start), std::get<1>(last_start),
				     "see previous start");
			} else {
				last_start = {line_no, column_from(text, start_match),
				              start_match.get<0>().to_string()};
			}
			inside_exclude = true;
		}

		if (inside_exclude || matches_line(text)) {
			exclude(line_no);
		}

		if (switch_off) {
			inside_exclude = false;
		}
	}

	void excludes::after_lines() {
		if (inside_exclude) {
			std::string_view start = std::get<2>(last_start);
			warn(std::get<0>(last_start), std::get<1>(last_start),
			     fmt::format("{} not matched with {}"sv, start,
			                 stop_for(start, "_START"sv)));
		}
	}

	void excludes::exclude(unsigned line) {
		if (!result.empty() && result.back().end + 1 == line) {
			result.back().end = line;
			return;
		}
		result.push_back({line, line});
	}

	void excludes::warn(unsigned line, unsigned column, std::string_view msg) {
		message(line, column, "\033[1;35mwarning"sv, msg);
	}

	void excludes::note(unsigned line, unsigned column, std::string_view msg) {
		message(line, column, "\033[1;36mnote"sv, msg);
	}

	void excludes::message(unsigned line,
	                       unsigned column,
	                       std::string_view tag,
	                       std::string_view msg) {
		fmt::print(stderr, "\033[1;37m{}:{}:{}:\033[m {}:\033[m {}\n", path,
		           line, column, tag, msg);
	}

	std::string excludes::stop_for(std::string_view start,
	                               std::string_view suffix) {
		return fmt::format("{}_STOP",
		                   start.substr(0, start.length() - suffix.length()));
	}
}  // namespace cov::app::strip
