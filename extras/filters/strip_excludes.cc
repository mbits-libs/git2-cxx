// Copyright (c) 2024 Marcin Zdun
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
#include <optional>
#include <set>
#include <span>
#include <string>
#include <strip/excludes.hh>
#include <strip/strip.hh>
#include "parser.hh"

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
			auto const chars = std::string_view{
			    reinterpret_cast<char const*>(stg.data()), stg.size()};
			return find_blocks(get_u8path(full_path), valid_markers, chars);
		}

		return {};
	}

	std::vector<exclude_report_line> find_lines(
	    json::map& line_coverage,
	    std::span<excl_block const> const& excludes,
	    std::string_view path,
	    std::filesystem::path src_dir) {
		auto const full_path = src_dir / make_u8path(path);
		auto const opened = io::fopen(full_path, "rb");
		if (!opened) return {};

		auto stg = opened.read();
		auto const chars = std::string_view{
		    reinterpret_cast<char const*>(stg.data()), stg.size()};

		return find_lines(line_coverage, excludes, chars);
	}

	template <typename T>
	void print_item(T item) {
		fmt::print(stderr, "{}", item);
	}

	void print_item(std::string const& s) { fmt::print(stderr, "{}", s); }

	template <typename C>
	void print_all(ExcludesStrings const& tr,
	               std::string_view prompt,
	               C const& items) {
		auto const length = items.size();

		if (length > 0) fmt::print(stderr, "{}", prompt);

		for (size_t index = 0; index < length; ++index) {
			auto const sep =
			    index == 0 ? " "sv
			               : tr(index < length - 1 ? ExcludesLng::LIST_COMMA
			                                       : ExcludesLng::LIST_AND);
			fmt::print(stderr, "{}", sep);
			print_item(items[index]);
		}
		if (length > 0) fmt::print(stderr, ".\n");
	}

	void missing_key(ExcludesStrings const& tr, std::string_view key) {
		fmt::print(stderr, "strip-excludes: {}\n",
		           fmt::format(
		               fmt::runtime(tr(ExcludesLng::REPORT_MISSING_KEY)), key));
	}

	int tool(args::args_view const& arguments) {
		parser p{arguments,
		         {platform::filters::locale_dir(), ::lngs::system_locales()}};
		p.parse();

		std::vector<std::string_view> valid_markers{*p.os, *p.compiler};
		if (*p.compiler == "clang"sv)
			valid_markers.push_back("llvm"sv);
		else if (*p.compiler == "llvm"sv)
			valid_markers.push_back("clang"sv);

		if (p.verbose > detail::none) {
			fmt::print(stderr, "verbose: ON\n");
			print_all(p.tr(),
			          fmt::format("{}:", p.tr()(ExcludesLng::DETAILS_MARKERS)),
			          valid_markers);
		}

		auto const text = platform::read_input();
		auto root = json::read_json({text.data(), text.size()});
		auto cvg = json::cast<json::map>(root);
		if (!cvg) return 1;

		git::init memory{};

		// TODO: after first major release, add code for compatibility check
		// through $schema
		auto head = json::cast_from_json<json::string>(root, u8"git.head"sv);
		auto files = json::cast_from_json<json::array>(root, u8"files"sv);
		if (!files || !head) {
			if (!head) {
				missing_key(p.tr(), "git/head"sv);
			}
			if (!files) {
				missing_key(p.tr(), "files"sv);
			}
			return 1;
		}

		auto const src_dir =
		    std::filesystem::weakly_canonical(make_u8path(p.src_dir));

		if (p.verbose > detail::none) {
			fmt::print(stderr, "{}: {}\n", p.tr()(ExcludesLng::DETAILS_SOURCES),
			           get_u8path(src_dir));
		}

		size_t line_counter = 0, fn_counter = 0, br_counter = 0;

		std::vector<exclude_report> excluded_lines{};
		if (p.verbose > detail::stats) {
			excluded_lines.reserve(files->size());
		}

		auto file_node_counter = std::numeric_limits<unsigned>::max();
		for (auto& file_node : *files) {
			++file_node_counter;
			auto file = json::cast<json::map>(file_node);
			if (!file) continue;

			auto json_file_name = json::cast<json::string>(file, u8"name");
			auto json_file_digest = json::cast<json::string>(file, u8"digest");
			auto json_file_lines =
			    json::cast<json::map>(file, u8"line_coverage");

			if (!json_file_name || !json_file_digest || !json_file_lines) {
				if (!json_file_name) {
					missing_key(p.tr(), fmt::format("files[{}]/name",
					                                file_node_counter));
				}
				if (!json_file_digest) {
					missing_key(p.tr(), fmt::format("files[{}]/digest",
					                                file_node_counter));
				}
				if (!json_file_lines) {
					missing_key(p.tr(), fmt::format("files[{}]/line_coverage",
					                                file_node_counter));
				}
				continue;
			}

			auto const file_name = from_u8s(*json_file_name);
			auto [excludes, empties] =
			    find_excl_blocks(valid_markers, file_name, src_dir);
			if (excludes.empty() && empties.empty()) continue;

			std::sort(excludes.begin(), excludes.end());

			if (p.verbose > detail::stats && !excludes.empty()) {
				excluded_lines.emplace_back(
				    file_name,
				    find_lines(*json_file_lines, excludes, file_name, src_dir));
			}

			line_counter += erase_lines(*json_file_lines, excludes, empties);
		}

		// VERBOSE
		// ===========================================================

		size_t counter_width = 0;
		for (auto const& [filename, lines] : excluded_lines) {
			for (auto const& [_1, counter, _2] : lines) {
				counter_width = std::max(counter_width, counter.length());
			}
		}

		for (auto const& [filename, lines] : excluded_lines) {
			unsigned prev{0};
			auto first{true};

			for (auto const& [line, counter, text] : lines) {
				if (first || (line - prev) > 1) {
#ifdef _WIN32
					fmt::print(stderr, "{}({})\n", filename, line);
#else
					fmt::print(stderr, "-- {}:{}\n", filename, line);
#endif
				}
				first = false;
				prev = line;

				fmt::print(stderr, "     {:>{}} | \033[2;49;30m{}\033[m\n",
				           counter, counter_width, text);
			}
		}

		// SUMMARY
		// ===========================================================
		if (line_counter || fn_counter || br_counter) {
			std::vector<std::string> stats{};
			stats.reserve(3);
			auto const format = [&stats, &tr = p.tr()](ExcludesCounted value,
			                                           size_t count) {
				stats.push_back(fmt::format(
				    fmt::runtime(tr(value, static_cast<intmax_t>(count))),
				    count));
			};

			if (line_counter) {
				format(ExcludesCounted::DETAILS_EXCLUDED_LINES, line_counter);
			}
			// GCOV_EXCL_START -- TODO: [FUNCTIONS] Enable for next task
			if (fn_counter) {
				format(ExcludesCounted::DETAILS_EXCLUDED_FUNCTIONS, fn_counter);
			}
			// GCOV_EXCL_STOP
			// GCOV_EXCL_START -- TODO: [BRANCHES] Enable for next task
			if (br_counter) {
				format(ExcludesCounted::DETAILS_EXCLUDED_BRANCHES, br_counter);
			}
			// GCOV_EXCL_STOP

			print_all(p.tr(),
			          fmt::format("strip-excludes: {}",
			                      p.tr()(ExcludesLng::DETAILS_EXCLUDED)),
			          stats);
		} else {
			fmt::print(stderr, "strip-excludes: {}.\n",
			           p.tr()(ExcludesLng::DETAILS_NOTHING_EXCLUDED));
		}

		json::write_json(stdout, root, json::concise);
		return 0;
	}
};  // namespace cov::app::strip

int tool(args::args_view const& arguments) {
	return cov::app::strip::tool(arguments);
}
