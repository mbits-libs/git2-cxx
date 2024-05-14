// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "strip/strip.hh"
#include <fmt/format.h>
#include <cov/io/file.hh>
#include <native/str.hh>
#include <numeric>

using namespace std::literals;

namespace cov::app::strip {
	exclude_result find_blocks(std::string const& path,
	                           std::span<std::string_view const> valid_markers,
	                           std::string_view text) {
		excludes builder{.path = path, .valid_markers = valid_markers};

		split(text, '\n', [&](unsigned line_no, std::string_view text) {
			builder.on_line(line_no, text);
		});
		builder.after_lines();
		return {std::move(builder.result), std::move(builder.empties)};
	}

	bool is_line_excluded(unsigned line_no,
	                      std::span<excl_block const> const& excludes) {
		for (auto const& [start, end] : excludes) {
			if (start <= line_no && line_no <= end) {
				return true;
			}
		}
		return false;
	}

	std::vector<exclude_report_line> find_lines(
	    json::map& line_coverage,
	    std::span<excl_block const> const& excludes,
	    std::string_view text) {
		std::vector<exclude_report_line> result{};

		size_t size =
		    std::transform_reduce(excludes.begin(), excludes.end(), size_t{},
		                          std::plus<>{}, [](auto const& exclude) {
			                          return exclude.end - exclude.start + 1;
		                          });

		result.reserve(size);

		split(text, '\n', [&](unsigned line_no, std::string_view text) {
			if (!is_line_excluded(line_no, excludes)) return;

			auto counter = ""s;

			auto const str = fmt::format("{}", line_no);
			auto key = to_u8s(str);
			auto it = line_coverage.find(key);

			if (it != line_coverage.end()) {
				auto json_counter = cast<long long>(it->second);
				if (json_counter) counter = fmt::format("{}", *json_counter);
			}

			result.emplace_back(line_no, std::move(counter),
			                    std::string{text.data(), text.size()});
		});

		std::sort(result.begin(), result.end());

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	static bool erase_line(json::map& line_coverage,
	                       unsigned line,
	                       which_lines intensity) {
		auto const str = fmt::format("{}", line);
		auto key = to_u8s(str);
		auto it = line_coverage.find(key);
		if (it != line_coverage.end()) {
			if (intensity == hard || it->second == json::node{0}) {
				line_coverage.erase(key);
				return true;
			}
		}
		return false;
	}  // GCOV_EXCL_LINE[GCC]

	unsigned erase_lines(json::map& line_coverage,
	                     std::span<excl_block const> excludes,
	                     std::set<unsigned> const& empties) {
		auto line_counter = 0u;
		for (auto const [start, stop] : excludes) {
			for (auto line = start; line <= stop; ++line) {
				if (erase_line(line_coverage, line, hard)) ++line_counter;
			}
		}

		for (auto const empty : empties) {
			if (erase_line(line_coverage, empty, soft)) ++line_counter;
		}
		return line_counter;
	}

	excl_block mask_range(json::map const& range,
	                      std::span<excl_block const> excludes,
	                      std::set<unsigned> const& empties) {
		auto json_range_start = cast<long long>(range, u8"start_line");
		auto json_range_end = cast<long long>(range, u8"end_line");
		if (!json_range_start) return {.start = 1, .end = 0};
		auto start_line = *json_range_start;
		auto end_line = json_range_end ? *json_range_end : start_line;

		for (auto const& block : excludes) {
			auto const block_start = static_cast<long long>(block.start);
			auto const block_end = static_cast<long long>(block.end);

			if (end_line < block_start || start_line > block_end) continue;
			if (start_line >= block_start) start_line = block_end + 1;
			if (end_line <= block_end) end_line = block_start - 1;
			if (end_line < start_line) break;
		}

		for (auto const line : empties) {
			if (end_line < start_line) break;
			if (start_line == line) ++start_line;
			if (end_line == line) --end_line;
		}

		return {.start = static_cast<unsigned>(start_line),
		        .end = static_cast<unsigned>(end_line)};
	}

	size_t filter_blocks(json::array* array,
	                     std::span<excl_block const> excludes,
	                     std::set<unsigned> const& empties) {
		json::array visible{};
		size_t counter{};
		visible.reserve(array->size());
		for (auto& node_range : *array) {
			auto json_range = cast<json::map>(node_range);
			if (!json_range) {
				++counter;  // GCOV_EXCL_LINE -- oh, c'mon, continue is
				            // counted...
				continue;
			}
			auto const block = mask_range(*json_range, excludes, empties);
			if (block.start > block.end) {
				++counter;
				continue;
			}

			visible.emplace_back(std::move(node_range));
		}
		*array = std::move(visible);
		return counter;
	}
}  // namespace cov::app::strip
