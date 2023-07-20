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

#define HEADER(HASH)          \
	"%C(yellow)report %" HASH \
	"R%Creset%md%n"           \
	"commit %" HASH "G%n"
#define RAW_PARENT_HEADER              \
	"%C(yellow)report %HR%Creset%md%n" \
	"parent %HP%n"
#define RAW_HEADER "%C(yellow)report %HR%Creset%md%n"

#define BUILD_HEADER(HASH) "%{B[build  %" HASH "1 [%pPL]%md%n%]}"

#define SHORT_LOG_(ALIGN)                               \
	"GitBranch:" ALIGN                                  \
	"%rD%n"                                             \
	"Coverage: " ALIGN                                  \
	"[%pPL] %C(faint normal)%pVL/%pTL%Creset %C(faint " \
	"rating)(%prL)%Creset%n"                            \
	"%{?pTF[Functions:" ALIGN                           \
	"[%pPF] %C(faint normal)%pVF/%pTF%Creset %C(faint " \
	"rating)(%prB)%Creset%n%]}"                         \
	"%{?pTB[Branches: " ALIGN                           \
	"[%pPB] %C(faint normal)%pVB/%pTB%Creset %C(faint " \
	"rating)(%prB)%Creset%n%]}"                         \
	"Author:   " ALIGN "%an <%ae>%n"
#define SHORT_LOG SHORT_LOG_(" ")

#define MEDIUM_LOG  \
	SHORT_LOG_(" ") \
	"Added:     %rd%n"

#define FULL_LOG    \
	SHORT_LOG_(" ") \
	"Commit:    %cn <%ce>%n"

#define FULL_BUILD(HASH)                                            \
	"%{B["                                                          \
	"- %C(yellow)build     %" HASH                                  \
	"1%Creset%n"                                                    \
	"%{?[  Config:   %mD%n%]}"                                      \
	"  Coverage: [%pPL] %C(faint normal)%pVL/%pTL%Creset %C(faint " \
	"rating)(%prL)%Creset%n"                                        \
	"%]}"

#define FULLER_LOG            \
	SHORT_LOG_("  ")          \
	"Commit:     %cn <%ce>%n" \
	"CommitDate: %cd%n"       \
	"Added:      %rd%n"

#define RAW_LOG                                   \
	"files %HF%n"                                 \
	"added %rt%n"                                 \
	"stats %pL %pTL %pVL, %pTF %pVF, %pTB %pTB%n" \
	"branch %rD%n"                                \
	"commit %HG%n"                                \
	"author %an <%ae>%n"                          \
	"committer %cn <%ce> %ct%n"

#define MESSAGE "%n%w(76,6,6)%B%n"

#define SHORT_MSG(HASH) HEADER(HASH) BUILD_HEADER(HASH) SHORT_LOG "%n%w()%s%n"sv
#define MEDIUM_MSG(HASH) HEADER(HASH) BUILD_HEADER(HASH) MEDIUM_LOG MESSAGE ""sv
#define FULL_MSG(HASH) HEADER(HASH) FULL_LOG FULL_BUILD(HASH) MESSAGE ""sv
#define FULLER_MSG(HASH) HEADER(HASH) FULLER_LOG FULL_BUILD(HASH) MESSAGE ""sv

	static constexpr format_descr format_descriptions[] = {
	    {
	        "oneline"sv,
	        "%C(yellow)%hR%Creset%md %C(bg rating) %pPL %Creset %s %C(red)%hG@%rD%Creset"sv,
	        "%C(yellow)%HR%Creset%md %C(bg rating) %pPL %Creset %s %C(red)%HG@%rD%Creset"sv,
	    },
	    {"short"sv, SHORT_MSG("h"), SHORT_MSG("H")},
	    {"medium"sv, MEDIUM_MSG("h"), MEDIUM_MSG("H")},
	    {"full"sv, FULL_MSG("h"), FULL_MSG("H")},
	    {"fuller"sv, FULLER_MSG("h"), FULLER_MSG("H")},
	    {
	        // here, both use abbrev-hash
	        "reference"sv,
	        "%C(yellow)%hR%Creset (%s, %rs) %C(red)%hG@%rD%Creset"sv,
	        "%C(yellow)%hR%Creset (%s, %rs) %C(red)%hG@%rD%Creset"sv,
	    },
	    {
	        "raw"sv,
	        ""sv,
	        ""sv,
	    },
	};

	void show_range::add_args(::args::parser& p,
	                          LogStrings const& tr,
	                          bool for_log) {
		if (for_log) {
			p.custom(oneline(), "oneline")
			    .help(tr(loglng::ONELINE_DESCRIPTION))
			    .opt();
		}
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
		p.set<std::true_type>(show_prop_names, "prop-names")
		    .help(tr(loglng::PROP_NAMES_DESCRIPTION))
		    .opt();
		p.set<std::false_type>(show_prop_names, "no-prop-names")
		    .help(tr(loglng::NO_PROP_NAMES_DESCRIPTION))
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
		auto env = placeholder::environment::from(repo, color_type,
		                                          selected_decorate());
		env.prop_names = show_prop_names;

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

				auto const has_parent = !report->parent_id().is_zero();
				if (has_parent != with_parent) {
					with_parent = has_parent;
					format =
					    formatter::from(with_parent ? RAW_PARENT : RAW_ROOT);
				}

				auto facade = placeholder::object_facade::present_report(
				    report.get(), &repo);
				std::puts(format.format(facade.get(), env).c_str());
				id = report->parent_id().id;
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

			auto facade =
			    placeholder::object_facade::present_report(report.get(), &repo);
			std::puts(format.format(facade.get(), env).c_str());
			id = report->parent_id().id;
		}
	}
}  // namespace cov::app
