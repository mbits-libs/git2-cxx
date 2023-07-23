// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <concepts>
#include <cov/app/show_range.hh>
#include <cov/format.hh>
#include <cov/repository.hh>

namespace cov::app {

	struct format_descr {
		std::string_view name;
		std::string_view short_format;
		std::string_view long_format;
	};

#define HEADER(HASH)       \
	"%C(yellow)%Ln %" HASH \
	"R%Creset%md%n"        \
	"%{?H4[%L4 %" HASH "4%n%]}"

#define ALIGN_MARK "\xFF"

#define BUILD_LIST(HASH) "%{B[build  %" HASH "1 [%pPL]%mz%n%]}"

#define COVERAGE_(LABEL, CODE)                                 \
	LABEL "[%pP" CODE "] %C(faint normal)%pV" CODE "/%pT" CODE \
	      "%Creset %C(faint "                                  \
	      "rating:" CODE ")(%pr" CODE ")%Creset%n"
#define HIDDEN_COVERAGE_(LABEL, CODE) \
	"%{?pT" CODE "[" COVERAGE_(LABEL, CODE) "%]}"

#define SHORT_LOG_                                            \
	"%{?git["                                                 \
	"GitBranch: " ALIGN_MARK                                  \
	"%rD%n"                                                   \
	"%]}" /* ?git */                                    /* */ \
	    COVERAGE_("Coverage: " ALIGN_MARK, "L")         /* */ \
	    HIDDEN_COVERAGE_("Functions: " ALIGN_MARK, "F") /* */ \
	    HIDDEN_COVERAGE_("Branches: " ALIGN_MARK, "B")  /* */ \
	    "%{?git["                                             \
	    "Author: " ALIGN_MARK                                 \
	    "%an <%ae>%n"                                         \
	    "%]}" /* ?git */
#define SHORT_LOG SHORT_LOG_

#define MEDIUM_LOG \
	SHORT_LOG_     \
	"%{?rd[Added: " ALIGN_MARK "%rd%n%]}"

#define FULL_LOG \
	SHORT_LOG_   \
	"%{?git[Commit: " ALIGN_MARK "%cn <%ce>%n%]}"

#define FULL_BUILD(HASH)                                \
	"%{B["                                              \
	"- %C(yellow)build " ALIGN_MARK "%" HASH            \
	"1%Creset%n"                                        \
	"%{?["                                              \
	"  Config: " ALIGN_MARK                             \
	"%mZ%n"                                             \
	"%]}"                                               \
	"  Coverage: " ALIGN_MARK                           \
	"[%pPL] %C(faint normal)%pVL/%pTL%Creset %C(faint " \
	"rating)(%prL)%Creset%n"                            \
	"%]}"                                               \
	"%{?prop["                                          \
	"Config: " ALIGN_MARK                               \
	"%mZ(8)%n"                                          \
	"%]}"

#define FULLER_LOG            \
	SHORT_LOG_                \
	"%{?git["                 \
	"Commit: " ALIGN_MARK     \
	"%cn <%ce>%n"             \
	"CommitDate: " ALIGN_MARK \
	"%cd%n"                   \
	"%]}"                     \
	"%{?rd["                  \
	"Added: " ALIGN_MARK      \
	"%rd%n"                   \
	"%]}"

#define RAW_MSG                                            \
	"%C(yellow)%Ln %HR%Creset%md%n"                        \
	"%{?H3[%L3 %H3%n%]}"                                   \
	"%{?H2[%L2 %H2%n%]}"                                   \
	"%{B["                                                 \
	"build %H1 %pL %pTL %pVL, %pTF %pVF, %pTB %pTB%n"      \
	"%{?[      %mZ%n%]}"                                   \
	"%]}"                                                  \
	"%{?rd[added %rt%n%]}"                                 \
	"%{?pT[stats %pL %pTL %pVL, %pTF %pVF, %pTB %pTB%n%]}" \
	"%{?git[branch %rD%n"                                  \
	"commit %HG%n"                                         \
	"author %an <%ae>%n"                                   \
	"committer %cn <%ce> %ct%n%]}"                         \
	"%{?prop[props %Z%n%]}" MESSAGE ""sv

#define MESSAGE "%{?git[%n%w(76,6,6)%B%n%]}"

#define SHORT_MSG(HASH) HEADER(HASH) BUILD_LIST(HASH) SHORT_LOG "%n%w()%s%n"sv
#define MEDIUM_MSG(HASH) HEADER(HASH) BUILD_LIST(HASH) MEDIUM_LOG MESSAGE ""sv
#define FULL_MSG(HASH) HEADER(HASH) FULL_LOG FULL_BUILD(HASH) MESSAGE ""sv
#define FULLER_MSG(HASH) HEADER(HASH) FULLER_LOG FULL_BUILD(HASH) MESSAGE ""sv

	static constexpr format_descr format_descriptions[] = {
	    {
	        "oneline"sv,
	        "%C(yellow)%hR%Creset%md %C(bg rating) %pPL %Creset%{?git[ %s %C(red)%hG@%rD%Creset%]}"sv,
	        "%C(yellow)%HR%Creset%md %C(bg rating) %pPL %Creset%{?git[ %s %C(red)%HG@%rD%Creset%]}"sv,
	    },
	    {"short"sv, SHORT_MSG("h"), SHORT_MSG("H")},
	    {"medium"sv, MEDIUM_MSG("h"), MEDIUM_MSG("H")},
	    {"full"sv, FULL_MSG("h"), FULL_MSG("H")},
	    {"fuller"sv, FULLER_MSG("h"), FULLER_MSG("H")},
	    {
	        // here, both use abbrev-hash
	        "reference"sv,
	        "%C(yellow)%hR%Creset%{?git[ (%s, %rs) %C(red)%hG@%rD%Creset%]}"sv,
	        "%C(yellow)%hR%Creset%{?git[ (%s, %rs) %C(red)%hG@%rD%Creset%]}"sv,
	    },
	    {"raw"sv, RAW_MSG, RAW_MSG},
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

	template <typename OnIter>
	void navigate(cov::repository const& repo,
	              cov::revs const& range,
	              std::optional<unsigned> max_count,
	              OnIter&& on_iter) {
		auto id = range.to;
		while (!id.is_zero() && id != range.from &&
		       (!max_count || *max_count)) {
			if (max_count) --*max_count;

			auto facade = placeholder::object_facade::present_oid(id, repo);
			if (!facade) break;

			on_iter(facade.get());
			auto parent = facade->parent_id();
			id = parent ? *parent : git::oid{};
		}
	}

	static void print_facade(formatter const& format,
	                         placeholder::environment const& env,
	                         placeholder::object_facade* facade,
	                         bool rstrip) {
		auto msg = format.format(facade, env);
		if (rstrip) {
			auto view = std::string_view{msg};
			while (!view.empty() &&
			       std::isspace(static_cast<unsigned char>(view.back())))
				view = view.substr(0, view.length() - 1);
			if (view.length() != msg.length()) msg.assign(view);
		}
		auto pos = env.add_align_marks ? msg.find('\xFF') : std::string::npos;
		if (pos == std::string::npos) {
			std::puts(msg.c_str());
			return;
		}

#if 0
		while (pos != std::string::npos) {
			msg[pos] = '*';
			pos = msg.find('\xFF', pos + 1);
		}
		std::puts(msg.c_str());
#else
		std::vector<std::pair<size_t, size_t> > inserts;

		while (pos != std::string::npos) {
			auto const new_line = msg.rfind('\n', pos);
			inserts.emplace_back(
			    new_line == std::string::npos ? 0 : new_line + 1, pos);
			pos = msg.find('\xFF', pos + 1);
		}

		auto length = msg.length() - inserts.size();
		size_t alignment = 0;

		for (auto const& [line_start, mark_pos] : inserts) {
			auto const width = mark_pos - line_start;
			alignment = std::max(alignment, width);
			length += alignment - width;
		}

		std::string aligned{};
		aligned.reserve(length);

		pos = 0;
		for (auto const& [line_start, mark_pos] : inserts) {
			auto const width = mark_pos - line_start;
			auto const missing = alignment - width;
			aligned.append(std::string_view{msg}.substr(pos, mark_pos - pos));
			for (size_t index = 0; index < missing; ++index)
				aligned.push_back(' ');
			pos = mark_pos + 1;
		}
		aligned.append(std::string_view{msg}.substr(pos));

		std::puts(aligned.c_str());
#endif
	}

	void show_range::print(cov::repository const& repo,
	                       cov::revs const& range,
	                       std::optional<unsigned> max_count,
	                       bool rstrip) const {
		auto env = placeholder::environment::from(repo, color_type,
		                                          selected_decorate());
		env.prop_names = show_prop_names;
		env.add_align_marks = selected_format != known_format::custom;

		auto format = formatter::from(format_str());
		navigate(repo, range, max_count,
		         [&format, &env, rstrip](placeholder::object_facade* facade) {
			         print_facade(format, env, facade, rstrip);
		         });
	}

	void show_range::print(cov::repository const& repo,
	                       cov::placeholder::object_facade& facade,
	                       bool rstrip) const {
		auto env = placeholder::environment::from(repo, color_type,
		                                          selected_decorate());
		env.prop_names = show_prop_names;
		env.add_align_marks = selected_format != known_format::custom;

		auto format = formatter::from(format_str());
		print_facade(format, env, &facade, rstrip);
	}

}  // namespace cov::app
