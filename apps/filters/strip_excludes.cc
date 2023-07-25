// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>
#include <cov/app/path.hh>
#include <cov/git2/commit.hh>
#include <cov/git2/global.hh>
#include <cov/git2/odb.hh>
#include <cov/git2/repository.hh>
#include <cov/git2/tree.hh>
#include <cov/io/file.hh>
#include <cov/version.hh>
#include <json/json.hpp>
#include <native/platform.hh>
#include <native/str.hh>
#include <native/strip_excludes/excludes.hh>
#include <native/strip_excludes/parser.hh>
#include <optional>
#include <set>
#include <span>
#include <string>

using namespace std::literals;

namespace cov::app::strip {
	exclude_result find_excl_blocks(
	    std::span<std::string_view const> valid_markers,
	    std::string_view path,
	    std::filesystem::path src_dir) {
		std::error_code ec{};

		auto const full_path = src_dir / make_u8path(path);
		auto const opened = io::fopen(full_path, "rb");
		if (opened) {
			auto stg = opened.read();
			excludes builder{.path = get_u8path(full_path),
			                 .valid_markers = valid_markers};

			auto const chars = std::string_view{
			    reinterpret_cast<char const*>(stg.data()), stg.size()};
			split(chars, '\n', [&](unsigned line_no, std::string_view text) {
				builder.on_line(line_no, text);
			});
			builder.after_lines();
			auto tmp = exclude_result{std::move(builder.result),
			                          std::move(builder.empties)};
			return tmp;
		}

		return {};
	}
	enum which_lines { soft = false, hard = true };
	bool non_zero(json::node const& n) {
		auto num = cast<long long>(n);
		return num && *num;
	}

	bool erase_line(json::map& line_coverage,
	                unsigned line,
	                which_lines intensity) {
		auto const str = fmt::format("{}", line);
		auto key = to_u8s(str);
		auto it = line_coverage.find(key);
		if (it != line_coverage.end()) {
			if (intensity == hard || it->second == json::node{0}) {
				line_coverage.erase(it);
				return true;
			}
		}
		return false;
	}

	excl_block mask_range(json::map const& range,
	                      std::span<cov::app::strip::excl_block const> blocks) {
		auto json_range_start = cast<long long>(range, u8"start_line");
		auto json_range_end = cast<long long>(range, u8"end_line");
		if (!json_range_start) return {.start = 1, .end = 0};
		auto start_line = *json_range_start;
		auto end_line = json_range_end ? *json_range_end : start_line;

		for (auto const& block : blocks) {
			auto const block_start = static_cast<long long>(block.start);
			auto const block_end = static_cast<long long>(block.end);

			if (end_line < block_start || start_line > block_end) continue;
			if (start_line >= block_start) start_line = block_end + 1;
			if (end_line <= block_end) end_line = block_start - 1;
			if (end_line < start_line) break;
		}
		return {.start = static_cast<unsigned>(start_line),
		        .end = static_cast<unsigned>(end_line)};
	}

	size_t filter_blocks(json::array* array,
	                     std::span<const cov::app::strip::excl_block> blocks) {
		json::array visible{};
		size_t counter{};
		visible.reserve(array->size());
		for (auto& node_range : *array) {
			auto json_range = cast<json::map>(node_range);
			if (!json_range) {
				++counter;
				continue;
			}
			auto const block = mask_range(*json_range, blocks);
			if (block.start > block.end) {
				++counter;
				continue;
			}
			visible.emplace_back(std::move(node_range));
		}
		*array = std::move(visible);
		return counter;
	}

	int tool(args::args_view const& arguments) {
		parser p{arguments, {platform::locale_dir(), ::lngs::system_locales()}};
		p.parse();

		std::vector<std::string_view> valid_markers{*p.os, *p.compiler};
		if (*p.compiler == "clang"sv)
			valid_markers.push_back("llvm"sv);
		else if (*p.compiler == "llvm"sv)
			valid_markers.push_back("clang"sv);

		auto const text = platform::read_input();
		auto root = json::read_json({text.data(), text.size()});
		auto cvg = json::cast<json::map>(root);
		if (!cvg) return 1;

		git::init memory{};

		// TODO: after first major release, add code for compatibility check
		// through $schema
		auto head = json::cast_from_json<json::string>(root, u8"git.head"sv);
		auto files = json::cast_from_json<json::array>(root, u8"files"sv);
		if (!files || !head) return 1;

		auto const src_dir =
		    std::filesystem::weakly_canonical(make_u8path(p.src_dir));

		size_t line_counter = 0, fn_counter = 0, br_counter = 0;

		for (auto& file_node : *files) {
			auto file = json::cast<json::map>(file_node);
			if (!file) continue;

			auto json_file_name = json::cast<json::string>(file, u8"name");
			auto json_file_digest = json::cast<json::string>(file, u8"digest");
			auto json_file_lines =
			    json::cast<json::map>(file, u8"line_coverage");

			if (!json_file_name || !json_file_digest || !json_file_lines)
				continue;

			auto [excludes, empties] = find_excl_blocks(
			    valid_markers, from_u8(*json_file_name), src_dir);
			if (excludes.empty() && empties.empty()) continue;

			std::sort(excludes.begin(), excludes.end());

			// LINES
			// ===========================================================
			for (auto const [start, stop] : excludes) {
				for (auto line = start; line <= stop; ++line) {
					if (erase_line(*json_file_lines, line, hard))
						++line_counter;
				}
			}

			for (auto const empty : empties) {
				if (erase_line(*json_file_lines, empty, soft)) ++line_counter;
			}

			// FUNCTIONS
			// ===========================================================

			if (auto json_array = json::cast<json::array>(file, u8"functions");
			    json_array) {
				fn_counter += filter_blocks(json_array, excludes);
			}

			// BRANCHES
			// ===========================================================

			if (auto json_array = json::cast<json::array>(file, u8"branches");
			    json_array) {
				br_counter += filter_blocks(json_array, excludes);
			}
		}

		// SUMMARY
		// ===========================================================
		if (line_counter || fn_counter || br_counter) {
			bool first = true;
			fmt::print(stderr, "strip-excludes: excluded ");
			if (line_counter) {
				first = false;
				fmt::print(stderr, "{} {}", line_counter,
				           line_counter == 1 ? "line"sv : "lines"sv);
			}
			if (fn_counter) {
				if (first) fmt::print(stderr, ", ");
				first = false;
				fmt::print(stderr, "{} {}", fn_counter,
				           fn_counter == 1 ? "function"sv : "functions"sv);
			}
			if (br_counter) {
				if (first) fmt::print(stderr, ", ");
				first = false;
				fmt::print(stderr, "{} {}", br_counter,
				           br_counter == 1 ? "branch"sv : "branches"sv);
			}
			fmt::print(stderr, "\n");
		}

		json::write_json(stdout, root, json::concise);
		return 0;
	}
};  // namespace cov::app::strip

int tool(args::args_view const& arguments) {
	return cov::app::strip::tool(arguments);
}
