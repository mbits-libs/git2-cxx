// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/app/args.hh>
#include <cov/app/cov_report_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/report.hh>
#include <cov/repository.hh>
#include <string>
#include <utility>
#include <vector>

namespace cov::app::builtin::report {
	using namespace app::report;

	struct new_head {
		std::string branch;
		git_oid tip;
	};

	struct parser : base_parser<errlng, replng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		struct parse_results {
			cov::repository repo{};
			report_info report{};
		};
		parse_results parse();

		std::string report_contents(git::repository_handle repo) const;

		std::string_view report_path() const noexcept { return report_; }
		std::string_view report_filter() const noexcept {
			return filter_ ? *filter_ : std::string_view{};
		}

		new_head update_current_branch(cov::repository& repo,
		                               git_oid const& file_list_id,
		                               git_info const& git,
		                               git_commit const& commit,
		                               io::v1::coverage_stats const& stats);

		void print_report(std::string_view local_branch,
		                  size_t files,
		                  cov::report const& report);

		template <typename Enum, typename... Args>
		[[noreturn]] void data_error(Enum id, Args&&... args)
		    const requires std::is_enum_v<Enum> {
			data_msg(str::args::lng::ERROR_MSG, id,
			         std::forward<Args>(args)...);
			std::exit(1);
		}  // GCOV_EXCL_LINE[WIN32]

		template <typename Enum, typename... Args>
		void data_warning(Enum id,
		                  Args&&... args) const requires std::is_enum_v<Enum> {
			data_msg(str::args::lng::WARNING_MSG, id,
			         std::forward<Args>(args)...);
		}

	private :
		// isolate this from acces to force clang-format to _not_ fix it to
		// "private : std::vector"
		std::vector<std::byte>
		filter(std::vector<std::byte> const& contents,
		       std::string_view filter,
		       std::filesystem::path const& cwd) const;
		// visual space

		template <typename Enum1, typename Enum2, typename... Args>
		void data_msg(Enum1 type, Enum2 id, Args&&... args)
		    const requires std::is_enum_v<Enum1> && std::is_enum_v<Enum2> {
			auto const vargs2 = fmt::make_format_args(args...);
			auto const message = fmt::vformat(tr_(id), vargs2);

			auto const vargs1 =
			    fmt::make_format_args(parser_.program(), message);
			auto const format = tr_(type);

			fmt::detail::is_utf8()
			    ? fmt::vprint(stderr, format, vargs1)
			    : fmt::detail::vprint_mojibake(stderr, format, vargs1);
			std::fputc('\n', stderr);
		}  // GCOV_EXCL_LINE[GCC]

		std::string quoted_list(std::span<std::string_view const> names);

		std::string report_{};
		std::optional<std::string> filter_{};
		bool amend_{};
	};

	struct stored_file {
		blob_info stg{};
		io::v1::coverage_stats stats{};
		git_oid cvg_id{};

		static stored_file from(git_commit const& commit,
		                        file_info const& info) {
			stored_file result{
			    .stg = commit.verify(info),
			};

			return result;
		}

		static std::vector<stored_file> from(git_commit const& commit,
		                                     std::vector<file_info> const& info,
		                                     parser const& p);
		void store(cov::repository& repo,
		           file_info const& info,
		           parser const& p);

		static bool store_tree(git_oid& id,
		                       cov::repository& repo,
		                       std::vector<file_info> const& report_files,
		                       std::vector<stored_file> const& files);

	private:
		bool store_coverage(cov::repository& repo, file_info const& info);
		bool store_contents(cov::repository& repo, git::bytes const& contents);
	};
}  // namespace cov::app::builtin::report
