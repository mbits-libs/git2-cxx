// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/show_range.hh>
#include <cov/format.hh>
#include <cov/repository.hh>

namespace cov::app {
	struct format_descr {
		std::string_view name;
		std::string_view short_format;
		std::string_view long_format;
	};

#define SHORT_HEADER                   \
	"%C(yellow)report %hr%Creset%md%n" \
	"commit %hc%n"
#define LONG_HEADER                    \
	"%C(yellow)report %Hr%Creset%md%n" \
	"commit %Hc%n"
#define RAW_PARENT_HEADER              \
	"%C(yellow)report %Hr%Creset%md%n" \
	"parent %rP%n"
#define RAW_HEADER "%C(yellow)report %Hr%Creset%md%n"

#define SHORT_LOG_(ALIGN)                            \
	"GitBranch:" ALIGN                               \
	"%rD%n"                                          \
	"Coverage: " ALIGN                               \
	"[%pP] %C(faint normal)%pC/%pR%Creset %C(faint " \
	"rating)(%pr)%Creset%n"                          \
	"Author:   " ALIGN "%an <%ae>%n"
#define SHORT_LOG SHORT_LOG_(" ")

#define MEDIUM_LOG  \
	SHORT_LOG_(" ") \
	"Added:     %rd%n"

#define FULL_LOG    \
	SHORT_LOG_(" ") \
	"Commit:    %cn <%ce>%n"

#define FULLER_LOG            \
	SHORT_LOG_("  ")          \
	"Commit:     %cn <%ce>%n" \
	"CommitDate: %cd%n"       \
	"Added:      %rd%n"

#define RAW_LOG           \
	"files %rF%n"         \
	"added %rt%n"         \
	"stats %pT %pR %pC%n" \
	"branch %rD%n"        \
	"commit %Hc%n"        \
	"author %an <%ae>%n"  \
	"committer %cn <%ce> %ct%n"

#define MESSAGE "%n%w(76,6,6)%B%n"

	static constexpr format_descr format_descriptions[] = {
	    {
	        "oneline"sv,
	        "%C(yellow)%hr%Creset%md %C(bg rating) %pP %Creset %s %C(red)%hc@%rD%Creset"sv,
	        "%C(yellow)%Hr%Creset%md %C(bg rating) %pP %Creset %s %C(red)%Hc@%rD%Creset"sv,
	    },
	    {
	        "short"sv,
	        SHORT_HEADER SHORT_LOG "%n%w()%s%n"sv,
	        LONG_HEADER SHORT_LOG "%n%w()%s%n"sv,
	    },
	    {
	        "medium"sv,
	        SHORT_HEADER MEDIUM_LOG MESSAGE ""sv,
	        LONG_HEADER MEDIUM_LOG MESSAGE ""sv,
	    },
	    {
	        "full"sv,
	        SHORT_HEADER FULL_LOG MESSAGE ""sv,
	        LONG_HEADER FULL_LOG MESSAGE ""sv,
	    },
	    {
	        "fuller"sv,
	        SHORT_HEADER FULLER_LOG MESSAGE ""sv,
	        LONG_HEADER FULLER_LOG MESSAGE ""sv,
	    },
	    {
	        // here, both use abbrev-hash
	        "reference"sv,
	        "%C(yellow)%hr%Creset (%s, %rs) %C(red)%hc@%rD%Creset"sv,
	        "%C(yellow)%hr%Creset (%s, %rs) %C(red)%hc@%rD%Creset"sv,
	    },
	    {
	        "raw"sv,
	        ""sv,
	        ""sv,
	    },
	};

	void show_range::add_args(::args::parser& p, LogStrings const& tr) {
		p.custom(oneline(), "oneline")
		    .help(tr(loglng::ONELINE_DESCRIPTION))
		    .opt();
		p.custom(select_format(), "format")
		    .meta(tr(loglng::FORMAT_META))
		    .help(tr(loglng::FORMAT_DESCRIPTION))
		    .opt();
		p.set<std::true_type>(abbrev_hash, "abbrev-hash")
		    .help(tr(loglng::ABBREV_HASH_DESCRIPTION))
		    .opt();
		p.set<std::false_type>(abbrev_hash, "no-abbrev-hash")
		    .help(tr(loglng::NO_ABBREV_HASH_DESCRIPTION))
		    .opt();
		p.arg(color_type, "color")
		    .meta(tr(loglng::WHEN_META))
		    .help(tr(loglng::WHEN_DESCRIPTION))
		    .opt();
		p.arg(decorate, "decorate")
		    .meta(tr(loglng::HOW_META))
		    .help(tr(loglng::HOW_DESCRIPTION))
		    .opt();
	}

	void show_range::select_format(std::string_view format) {
		for (auto const prefix : {"pretty:"sv, "format:"sv, "tformat:"sv}) {
			if (format.starts_with(prefix)) {
				selected_format = known_format::custom;
				custom_format.assign(format.substr(prefix.size()));
				return;
			}
		}

		for (unsigned index = 0; index < std::size(format_descriptions);
		     ++index) {
			auto const& descr = format_descriptions[index];
			if (format == descr.name) {
				selected_format = known_format{index};
				return;
			}
		}

		if (format.contains('%')) {
			selected_format = known_format::custom;
			custom_format.assign(format);
			return;
		}

		selected_format = known_format::medium;
	}

	std::string_view show_range::format_str() const noexcept {
		if (selected_format == known_format::custom) return custom_format;
		auto const& descr =
		    format_descriptions[static_cast<unsigned>(selected_format)];
		return abbrev_hash ? descr.short_format : descr.long_format;
	}

	decorate_feature show_range::selected_decorate() const noexcept {
		if (selected_format == known_format::custom) return use_feature::yes;
		return decorate;
	}

	void show_range::print(cov::repository const& repo,
	                       cov::revs const& range,
	                       std::optional<unsigned> max_count) const {
		using namespace std::chrono;
		auto ctx =
		    placeholder::context::from(repo, color_type, selected_decorate());

		auto id = range.to;
		std::error_code ec{};

		if (selected_format == known_format::raw) {
#define RAW_PARENT RAW_PARENT_HEADER RAW_LOG MESSAGE ""sv
#define RAW_ROOT RAW_HEADER RAW_LOG MESSAGE ""sv

			auto format = formatter::from(RAW_PARENT);
			bool with_parent = true;

			while (!git_oid_is_zero(&id) && git_oid_cmp(&id, &range.from) &&
			       (!max_count || *max_count)) {
				if (max_count) --*max_count;

				auto report = repo.lookup<cov::report>(id, ec);
				if (!report || ec) return;

				auto const has_parent =
				    git_oid_is_zero(&report->parent_report()) == 0;
				if (has_parent != with_parent) {
					with_parent = has_parent;
					format =
					    formatter::from(with_parent ? RAW_PARENT : RAW_ROOT);
				}

				auto const view = placeholder::report_view::from(*report);
				std::puts(format.format(view, ctx).c_str());
				id = report->parent_report();
			}
			return;
#undef RAW_PARENT
#undef RAW_ROOT
		}

		auto format = formatter::from(format_str());

		while (!git_oid_is_zero(&id) && git_oid_cmp(&id, &range.from) &&
		       (!max_count || *max_count)) {
			if (max_count) --*max_count;

			auto report = repo.lookup<cov::report>(id, ec);
			if (!report || ec) break;

			auto const view = placeholder::report_view::from(*report);
			std::puts(format.format(view, ctx).c_str());
			id = report->parent_report();
		}
	}
}  // namespace cov::app
