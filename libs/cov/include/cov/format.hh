// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/reference.hh>
#include <cov/report.hh>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace cov {
	enum translatable {
		in_the_future,
		seconds_ago,
		minutes_ago,
		hours_ago,
		days_ago,
		weeks_ago,
		months_ago,
		years_months_ago,  // uses months_ago
		years_ago,
		mark_failing,
		mark_incomplete,
		mark_passing,
	};
}  // namespace cov

namespace cov::placeholder {
	enum class color {
		normal,
		reset,
		red,
		green,
		yellow,
		blue,
		magenta,
		cyan,
		bold_normal,
		bold_red,
		bold_green,
		bold_yellow,
		bold_blue,
		bold_magenta,
		bold_cyan,
		faint_normal,
		faint_red,
		faint_green,
		faint_yellow,
		faint_blue,
		faint_magenta,
		faint_cyan,
		bg_red,
		bg_green,
		bg_yellow,
		bg_blue,
		bg_magenta,
		bg_cyan,
		rating,
		bold_rating,
		faint_rating,
		bg_rating,
	};

	struct width {
		unsigned total;
		unsigned indent1;
		unsigned indent2;
		constexpr auto operator<=>(width const&) const noexcept = default;
	};

	enum class report {
		hash,
		hash_abbr,
		parent_hash,
		parent_hash_abbr,
		file_list_hash,
		file_list_hash_abbr,
		ref_names,
		ref_names_unwrapped,
		magic_ref_names,
		magic_ref_names_unwrapped,
		branch,
		lines_percent,
		lines_total,
		lines_relevant,
		lines_covered,
		lines_rating,
	};

	enum class commit {
		hash,
		hash_abbr,
		subject,
		subject_sanitized,
		body,
		body_raw,
	};

	enum class person {
		name,
		email,
		email_local,
		date,
		date_relative,
		date_timestamp,
		date_iso_like,
		date_iso_strict,
		date_short,
	};
	enum class who { author, committer, reporter };
	using person_info = std::pair<who, person>;

	using format = std::
	    variant<std::string, char, color, width, report, commit, person_info>;

	using iterator = std::back_insert_iterator<std::string>;
	struct internal_context;

	struct refs {
		std::string HEAD;
		std::map<std::string, std::string> tags, heads;
		std::string HEAD_ref;
		iterator format(iterator out,
		                git_oid const* id,
		                bool wrapped,
		                bool magic_colors,
		                struct report_view const&,
		                internal_context&) const;
	};

	struct ratio {
		unsigned num, den;
		constexpr ratio gcd() const noexcept {
			auto const div = std::gcd(num, den);
			if (!div) return *this;
			return {.num = num / div, .den = den / div};
		}
	};

	struct rating {
		ratio incomplete;
		ratio passing;
	};

	struct context {
		sys_seconds now;
		unsigned hash_length;
		refs names;
		rating marks{.incomplete{75, 100}, .passing{9, 10}};
		std::string_view time_zone{};
		std::string_view locale{};
		void* app{};
		std::string (*translate)(long long count,
		                         translatable scale,
		                         void* app) = {};
		std::string (*colorize)(color, void* app) = {};
	};

	struct git_person {
		std::string_view name;
		std::string_view email;
		sys_seconds date;

		iterator format(iterator out, internal_context& ctx, person fld) const;
		static git_person from(cov::report const& report, who type) noexcept {
			if (type == who::author) {
				return {
				    report.author_name(),
				    report.author_email(),
				    report.commit_time_utc(),
				};
			} else {  // GCOV_EXCL_LINE[WIN32]
				return {
				    report.committer_name(),
				    report.committer_email(),
				    report.commit_time_utc(),
				};
			}
		}
	};

	struct git_commit_view {
		git_oid const* id{};
		std::string_view branch{};
		std::string_view message{};
		git_person author{}, committer{};

		iterator format(iterator out, internal_context& ctx, commit fld) const;
		iterator format(iterator out,
		                internal_context& ctx,
		                person_info const& pair) const {
			if (std::get<0>(pair) == who::author)
				return author.format(out, ctx, std::get<1>(pair));
			return committer.format(out, ctx, std::get<1>(pair));
		}
		static git_commit_view from(cov::report const& report) noexcept {
			return {
			    .id = &report.commit(),
			    .branch = report.branch(),
			    .message = report.message(),
			    .author = git_person::from(report, who::author),
			    .committer = git_person::from(report, who::committer),
			};
		}
	};

	struct report_view {
		git_oid const* id{};
		git_oid const* parent{};
		git_oid const* file_list{};
		sys_seconds date{};
		git_commit_view git{};
		io::v1::coverage_stats const* stats{};

		iterator format(iterator out, internal_context& ctx, report fld) const;
		iterator format(iterator out, internal_context& ctx, color fld) const;
		iterator format(iterator out, internal_context& ctx, commit fld) const {
			width_cleaner clean{ctx};
			return git.format(out, ctx, fld);
		}
		iterator format(iterator out,
		                internal_context& ctx,
		                person_info const& pair) const {
			width_cleaner clean{ctx};
			if (std::get<0>(pair) == who::reporter)
				return git_person{{}, {}, date}.format(out, ctx,
				                                       std::get<1>(pair));
			return git.format(out, ctx, pair);
		}
		iterator format_with(iterator out,
		                     internal_context& ctx,
		                     placeholder::format const& fmt) const;
		static report_view from(cov::report const& report) noexcept {
			return {
			    .id = &report.oid(),
			    .parent = &report.parent_report(),
			    .file_list = &report.file_list(),
			    .date = report.add_time_utc(),
			    .git = git_commit_view::from(report),
			    .stats = &report.stats(),
			};
		}

	private:
		struct width_cleaner {
			internal_context& ctx;
			~width_cleaner() noexcept;
		};
	};
}  // namespace cov::placeholder

namespace cov {
	class formatter {
	public:
		explicit formatter(std::vector<placeholder::format>&& format)
		    : format_{std::move(format)} {}

		static formatter from(std::string_view input);

		static std::string shell_colorize(placeholder::color, void*);

		std::string format(placeholder::report_view const&,
		                   placeholder::context const&);

		std::vector<placeholder::format> const& parsed() const noexcept {
			return format_;
		}

	private:
		std::vector<placeholder::format> format_{};
	};
}  // namespace cov
