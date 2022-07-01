// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <memory>
#include <string_view>

namespace cov {
	struct report : object {
		obj_type type() const noexcept override { return obj_report; };
		bool is_report() const noexcept final { return true; }
		virtual git_oid const* parent_report() const noexcept = 0;
		virtual git_oid const* file_list() const noexcept = 0;
		virtual git_oid const* commit() const noexcept = 0;
		virtual std::string_view branch() const noexcept = 0;
		virtual std::string_view author_name() const noexcept = 0;
		virtual std::string_view author_email() const noexcept = 0;
		virtual std::string_view committer_name() const noexcept = 0;
		virtual std::string_view committer_email() const noexcept = 0;
		virtual std::string_view message() const noexcept = 0;
		virtual git_time_t commit_time_utc() const noexcept = 0;
		virtual git_time_t add_time_utc() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;
	};

	ref_ptr<report> report_create(git_oid const& parent_report,
	                              git_oid const& file_list,
	                              git_oid const& commit,
	                              std::string const& branch,
	                              std::string const& author_name,
	                              std::string const& author_email,
	                              std::string const& committer_name,
	                              std::string const& committer_email,
	                              std::string const& message,
	                              git_time_t commit_time_utc,
	                              git_time_t add_time_utc,
	                              io::v1::coverage_stats const& stats);

	struct report_entry {
		virtual ~report_entry();
		virtual bool is_dirty() const noexcept = 0;
		bool in_workdir() const noexcept { return is_dirty(); }
		bool in_index() const noexcept { return !is_dirty(); }
		virtual bool is_modified() const noexcept = 0;
		virtual std::string_view path() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;
		virtual git_oid const* contents() const noexcept = 0;
		virtual git_oid const* line_coverage() const noexcept = 0;
	};

	struct report_entry_builder {
		bool is_dirty{};
		bool is_modified{};
		std::string path{};
		io::v1::coverage_stats stats{};
		git_oid contents{};
		git_oid line_coverage{};

		report_entry_builder& set_dirty(bool value = true) {
			is_dirty = value;
			return *this;
		}

		report_entry_builder& set_modifed(bool value = true) {
			is_modified = value;
			return *this;
		}

		report_entry_builder& set_path(std::string_view value) {
			path.assign(value);
			return *this;
		}

		report_entry_builder& set_stats(io::v1::coverage_stats const& value) {
			stats = value;
			return *this;
		}

		report_entry_builder& set_stats(uint32_t total,
		                                uint32_t relevant,
		                                uint32_t covered) {
			return set_stats({total, relevant, covered});
		}

		report_entry_builder& set_contents(git_oid const& value) {
			contents = value;
			return *this;
		}

		report_entry_builder& set_line_coverage(git_oid const& value) {
			line_coverage = value;
			return *this;
		}

		std::unique_ptr<report_entry> create() &&;
	};

	struct report_files : object {
		obj_type type() const noexcept override { return obj_report_files; };
		bool is_report_files() const noexcept final { return true; }
		virtual std::vector<std::unique_ptr<report_entry>> const& entries()
		    const noexcept = 0;
	};
	ref_ptr<report_files> report_files_create(
	    std::vector<std::unique_ptr<report_entry>>&&);

	struct line_coverage : object {
		obj_type type() const noexcept override { return obj_line_coverage; };
		bool is_line_coverage() const noexcept final { return true; }
		virtual std::vector<io::v1::coverage> const& coverage()
		    const noexcept = 0;
	};
	ref_ptr<line_coverage> line_coverage_create(
	    std::vector<io::v1::coverage>&&);
}  // namespace cov
