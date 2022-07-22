// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/db.hh>
#include <cov/io/types.hh>
#include <cov/report.hh>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace cov::testing {
	using line_cvg = std::map<unsigned, unsigned>;

	struct file {
		std::string_view name;
		bool dirty{}, modified{};
		line_cvg lines{};
		unsigned finish{};

		report_entry_builder build(io::v1::coverage_stats const& stats,
		                           git_oid const& lines_id) const {
			report_entry_builder bldr{};
			bldr.set_path(name)
			    .set_dirty(dirty)
			    .set_modifed(modified)
			    .set_stats(stats)
			    .set_line_coverage(lines_id);
			return bldr;
		}
	};

	struct git_commit {
		git_oid commit{};
		std::string branch{};
		std::string author_name{};
		std::string author_email{};
		std::string committer_name{};
		std::string committer_email{};
		std::string message{};
		sys_seconds commit_time_utc{};
	};

	struct report {
		git_oid parent{};
		sys_seconds add_time_utc{};
		git_commit head{};
		std::vector<file> files{};
	};

	inline ref_ptr<line_coverage> from_lines(line_cvg const& lines,
	                                         unsigned finish = 0) {
		size_t need = finish ? 1 : 0 + lines.size();
		unsigned prev_line = 0;
		for (auto&& [line, count] : lines) {
			auto const nulls = line - prev_line - 1;
			if (nulls) need += 1;
			prev_line = line;
		}

		std::vector<io::v1::coverage> result{};
		result.reserve(need);

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
		prev_line = 0;
		for (auto&& [line, count] : lines) {
			auto const nulls = line - prev_line - 1;
			if (nulls) result.push_back({.value = nulls, .is_null = 1});
			result.push_back({.value = count, .is_null = 0});
			prev_line = line;
		}
		if (finish) result.push_back({.value = finish, .is_null = 1});
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

		return line_coverage_create(std::move(result));
	}

	inline line_cvg from_coverage(std::vector<io::v1::coverage> const& lines,
	                              unsigned& finish) {
		line_cvg result{};
		unsigned line = 0;
		for (auto&& cvg : lines) {
			if (cvg.is_null) {
				line += cvg.value;
				continue;
			}
			line++;
			result[line] = cvg.value;
		}

		finish = 0;
		if (!lines.empty() && lines.back().is_null) finish = lines.back().value;

		return result;
	}

	inline io::v1::coverage_stats stats(
	    std::vector<io::v1::coverage> const& lines) {
		auto result = io::v1::coverage_stats::init();
		for (auto&& cvg : lines) {
			result += cvg;
		}
		return result;
	}
}  // namespace cov::testing
