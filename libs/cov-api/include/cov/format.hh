// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <chrono>
#include <cov/format_args.hh>
#include <cov/reference.hh>
#include <cov/report.hh>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace cov {
	struct repository;

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

namespace cov::platform {
	bool is_terminal(FILE* out) noexcept;
}

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

	enum class self {
		primary_hash,
		secondary_hash,
		tertiary_hash,
		quaternary_hash,
		primary_hash_abbr,
		secondary_hash_abbr,
		tertiary_hash_abbr,
		quaternary_hash_abbr,
	};

	enum class stats {
		lines,
		lines_percent,
		lines_total,
		lines_visited,
		lines_rating,
		functions_percent,
		functions_total,
		functions_visited,
		functions_rating,
		branches_percent,
		branches_total,
		branches_visited,
		branches_rating,
	};

	enum class report {
		ref_names,
		ref_names_unwrapped,
		magic_ref_names,
		magic_ref_names_unwrapped,
		branch,
	};

	enum class commit {
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

	template <typename Variant, typename Append>
	struct extend;
	template <typename... Variant, typename Append>
	struct extend<std::variant<Variant...>, Append> {
		using type = std::variant<Variant..., Append>;
	};

	using common = std::variant<std::string,
	                            char,
	                            color,
	                            width,
	                            self,
	                            stats,
	                            report,
	                            commit,
	                            person_info>;

	enum class block_type { loop_start, if_start, end };
	using block_token = std::pair<block_type, std::string>;

	using token = extend<common, block_token>::type;

	template <typename Variant>
	struct block_t {
		block_type type{};
		std::string ref{};
		std::vector<Variant> opcodes{};

		bool operator==(block_t const&) const noexcept = default;
	};

	struct printable : extend<common, block_t<printable>>::type {
		using base_type = extend<common, block_t<printable>>::type;
		using base_type::base_type;
	};

	using block = block_t<printable>;

	using iterator = std::back_insert_iterator<std::string>;
	struct internal_environment;

	struct refs {
		std::string HEAD{};
		std::map<std::string, std::string> tags{}, heads{};
		std::string HEAD_ref{};
		bool operator==(refs const&) const noexcept = default;
		iterator format(iterator out,
		                git::oid const* id,
		                bool wrapped,
		                bool magic_colors,
		                struct context const&,
		                internal_environment&) const;
	};

	struct ratio {
		unsigned num, den;

		bool operator==(ratio const&) const noexcept = default;
		constexpr ratio gcd() const noexcept {
			auto const div = std::gcd(num, den);
			if (!div) return *this;
			return {.num = num / div, .den = den / div};
		}
	};

	struct rating {
		ratio incomplete;
		ratio passing;
		bool operator==(rating const&) const noexcept = default;
	};

	struct environment {
		sys_seconds now{};
		unsigned hash_length{};
		refs names{};
		rating marks{.incomplete{75, 100}, .passing{9, 10}};
		std::string_view time_zone{};
		std::string_view locale{};
		void* app{};
		std::string (*translate)(long long count,
		                         translatable scale,
		                         void* app) = {};
		std::string (*colorize)(color, void* app) = {};
		bool decorate{false};
		bool prop_names{true};

		bool operator==(environment const&) const noexcept = default;
		static environment from(cov::repository const&,
		                        color_feature clr,
		                        decorate_feature decorate);
		static rating rating_from(cov::repository const&);
	};

	struct git_person {
		std::string_view name;
		std::string_view email;
		sys_seconds date;

		iterator format(iterator out,
		                internal_environment& env,
		                person fld) const;
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
		std::string_view branch{};
		std::string_view message{};
		git_person author{}, committer{};

		iterator format(iterator out,
		                internal_environment& env,
		                commit fld) const;
		iterator format(iterator out,
		                internal_environment& env,
		                person_info const& pair) const {
			if (std::get<0>(pair) == who::author)
				return author.format(out, env, std::get<1>(pair));
			return committer.format(out, env, std::get<1>(pair));
		}
		static git_commit_view from(cov::report const& report) noexcept {
			return {
			    .branch = report.branch(),
			    .message = report.message(),
			    .author = git_person::from(report, who::author),
			    .committer = git_person::from(report, who::committer),
			};
		}
	};

	struct object_list {
		virtual ~object_list();

		virtual std::unique_ptr<struct object_facade> next() = 0;
		virtual void reset() noexcept = 0;
	};

	struct object_facade {
		virtual ~object_facade();

		virtual std::unique_ptr<object_list> loop(std::string_view) = 0;
		virtual bool condition(std::string_view tag);
		virtual iterator prop(iterator,
		                      bool wrapped,
		                      bool magic_colors,
		                      internal_environment&) = 0;
		virtual git::oid const* primary_id() noexcept = 0;
		virtual git::oid const* secondary_id() noexcept = 0;
		virtual git::oid const* tertiary_id() noexcept = 0;
		virtual git::oid const* quaternary_id() noexcept = 0;
		virtual std::chrono::sys_seconds added() noexcept = 0;
		virtual io::v1::coverage_stats const* stats() noexcept = 0;
		virtual git_commit_view const* git() noexcept = 0;

		static std::unique_ptr<object_facade> present_report(
		    cov::report const*,
		    cov::repository const*);
		static std::unique_ptr<object_facade> present_build(
		    cov::report::build const*,
		    cov::repository const*);
		static std::unique_ptr<object_facade> present_build(
		    cov::build*,
		    cov::repository const*);
	};

	struct context {
		object_facade* facade{};

		context() = default;
		context(object_facade* facade) : facade{facade} {}

		iterator format(iterator out,
		                internal_environment& env,
		                color fld) const;
		iterator format(iterator out,
		                internal_environment& env,
		                self fld) const;
		iterator format(iterator out,
		                internal_environment& env,
		                placeholder::stats fld) const;
		iterator format(iterator out,
		                internal_environment& env,
		                report fld) const;
		iterator format(iterator out,
		                internal_environment& env,
		                commit fld) const;
		iterator format(iterator out,
		                internal_environment& env,
		                person_info const& pair) const;
		iterator format_with(iterator out,
		                     internal_environment& env,
		                     placeholder::printable const& fmt) const;
		iterator format_all(
		    iterator out,
		    internal_environment& env,
		    std::vector<placeholder::printable> const& opcodes) const;

		iterator format_properties(
		    iterator out,
		    internal_environment& env,
		    bool wrapped,
		    bool magic_colors,
		    std::map<std::string, cov::report::property> const& properties)
		    const;

	private:
		struct width_cleaner {
			internal_environment& env;
			~width_cleaner() noexcept;
		};
	};

}  // namespace cov::placeholder

namespace cov {
	class formatter {
	public:
		explicit formatter(std::vector<placeholder::printable>&& format);

		static formatter from(std::string_view input);
		static std::string shell_colorize(placeholder::color, void*);

		std::string format(placeholder::context const&,
		                   placeholder::environment const&) const;

		std::vector<placeholder::printable> const& parsed() const noexcept {
			return format_;
		}

		static translatable apply_mark(io::v1::stats const& stats,
		                               placeholder::rating const& marks);

		static placeholder::color apply_mark(placeholder::color color,
		                                     io::v1::stats const& stats,
		                                     placeholder::rating const& marks);

	private:
		static std::string no_translation(long long count,
		                                  translatable scale,
		                                  void*);

		std::vector<placeholder::printable> format_{};
		bool needs_timezones_{true};
	};
}  // namespace cov
