// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/io/types.hh>
#include <cov/repository.hh>
#include <map>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cov::projection {
	using io::v1::coverage_stats;

	struct label {
		std::string display{};
		std::string expanded{};
		bool operator==(label const&) const noexcept = default;
	};

	struct entry_stats {
		coverage_stats current{coverage_stats::init()};
		coverage_stats previous{coverage_stats::init()};
		bool operator==(entry_stats const&) const noexcept = default;

		void extend(entry_stats const& rhs) noexcept {
			current += rhs.current;
			previous += rhs.previous;
		}

		file_diff diff(unsigned char digits = 2) const noexcept {
			auto const curr = multi_ratio<>::calc(current, digits);
			auto const prev = multi_ratio<>::calc(previous, digits);
			return {
			    .coverage = {.current = curr,
			                 .diff = multi_ratio<>::diff(curr, prev)},
			    .stats = {.current = current,
			              .diff = io::v1::diff(current, previous)},
			};
		}
	};

	enum class entry_type { standalone_file, file, directory, module };

	struct entry {
		entry_type type;
		label name{};
		entry_stats stats{};
		std::string previous_name{};
		file_diff::kind diff_kind{};

		bool operator==(entry const&) const noexcept = default;
	};

	struct prefixed {
		std::string filter;
		std::string prefix{filter};

		prefixed(std::string const& filter_value,
		         std::string_view sep = {"/", 1})
		    : filter{filter_value} {
			if (filter.ends_with(sep)) {
				filter = filter.substr(0, filter.length() - sep.length());
			} else {
				if (!prefix.empty()) prefix.append(sep);
			}
		}

		bool prefixes(std::string_view name) const noexcept {
			return name.starts_with(prefix) || name == filter;
		}

		std::string apply_to(std::string_view name) const {
			if (name.empty()) return filter;
			auto copy = prefix;
			copy.append(name);
			return copy;
		}
	};

	struct report_filter {
		struct modules const* mods;
		std::string_view sep{module_separator()};
		prefixed module;
		prefixed fname;

		report_filter(struct modules const* mods,
		              std::string const& module_filter,
		              std::string const& fname_filter)
		    : mods{mods}, module{module_filter, sep}, fname{fname_filter} {}

		std::vector<entry> project(std::vector<file_stats> const& report) const;

	private:
		std::string_view module_separator() const noexcept;
		std::vector<file_stats const*> file_projection(
		    std::vector<file_stats> const& report) const;
		std::pair<std::map<std::string, std::unordered_set<std::string>>,
		          std::unordered_set<std::string>>
		modules_projection() const;
	};
}  // namespace cov::projection
