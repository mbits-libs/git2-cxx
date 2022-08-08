// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cov {
	struct object_with_id : object {
		virtual git_oid const& oid() const noexcept = 0;
	};

	struct report : object_with_id {
		obj_type type() const noexcept override { return obj_report; };
		bool is_report() const noexcept final { return true; }
		virtual git_oid const& parent_report() const noexcept = 0;
		virtual git_oid const& file_list() const noexcept = 0;
		virtual git_oid const& commit() const noexcept = 0;
		virtual std::string_view branch() const noexcept = 0;
		virtual std::string_view author_name() const noexcept = 0;
		virtual std::string_view author_email() const noexcept = 0;
		virtual std::string_view committer_name() const noexcept = 0;
		virtual std::string_view committer_email() const noexcept = 0;
		virtual std::string_view message() const noexcept = 0;
		virtual sys_seconds commit_time_utc() const noexcept = 0;
		virtual sys_seconds add_time_utc() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;

		struct signature_view {
			std::string_view name;
			std::string_view email;
		};

		static ref_ptr<report> create(git_oid const& parent_report,
		                              git_oid const& file_list,
		                              git_oid const& commit,
		                              std::string_view branch,
		                              signature_view author,
		                              signature_view committer,
		                              std::string_view message,
		                              sys_seconds commit_time_utc,
		                              sys_seconds add_time_utc,
		                              io::v1::coverage_stats const& stats) {
			return create({}, parent_report, file_list, commit, branch, author,
			              committer, message, commit_time_utc, add_time_utc,
			              stats);
		}

		static ref_ptr<report> create(git_oid const& oid,
		                              git_oid const& parent_report,
		                              git_oid const& file_list,
		                              git_oid const& commit,
		                              std::string_view branch,
		                              signature_view author,
		                              signature_view committer,
		                              std::string_view message,
		                              sys_seconds commit_time_utc,
		                              sys_seconds add_time_utc,
		                              io::v1::coverage_stats const& stats);
	};

	struct report_entry {
		virtual ~report_entry();
		virtual std::string_view path() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;
		virtual git_oid const& contents() const noexcept = 0;
		virtual git_oid const& line_coverage() const noexcept = 0;
	};

	struct report_files : object {
		obj_type type() const noexcept override { return obj_report_files; };
		bool is_report_files() const noexcept final { return true; }
		virtual std::vector<std::unique_ptr<report_entry>> const& entries()
		    const noexcept = 0;

		static ref_ptr<report_files> create(
		    std::vector<std::unique_ptr<report_entry>>&&);
	};

	class report_files_builder {
	public:
		struct info {
			std::string_view path{};
			io::v1::coverage_stats stats{};
			git_oid contents{};
			git_oid line_coverage{};
		};
		report_files_builder& add(std::unique_ptr<report_entry>&&);
		report_files_builder& add(std::string_view path,
		                          io::v1::coverage_stats const& stats,
		                          git_oid const& contents,
		                          git_oid const& line_coverage);
		report_files_builder& add_nfo(info const& nfo) {
			return add(nfo.path, nfo.stats, nfo.contents, nfo.line_coverage);
		}
		bool remove(std::string_view path);
		ref_ptr<report_files> extract();
		std::vector<std::unique_ptr<report_entry>> release();

	private:
		std::map<std::string_view, std::unique_ptr<report_entry>> entries_{};
	};

	struct line_coverage : object {
		obj_type type() const noexcept override { return obj_line_coverage; };
		bool is_line_coverage() const noexcept final { return true; }
		virtual std::vector<io::v1::coverage> const& coverage()
		    const noexcept = 0;

		static ref_ptr<line_coverage> create(std::vector<io::v1::coverage>&&);
	};

}  // namespace cov
