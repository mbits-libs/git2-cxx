// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/format.hh>
#include <ctime>
#include "path-utils.hh"

namespace cov::placeholder {
	namespace {
		struct leaky_iterator : iterator {
			auto cont() { return container; }
		};

		iterator format_str(iterator out, std::string_view view) {
			leaky_iterator{out}.cont()->append(view);
			return out;
		}

		std::string_view subject_from(std::string_view message) {
			return strip(message.substr(0, message.find('\n')));
		}

		std::string_view body_from(std::string_view message) {
			auto const line = message.find('\n');
			if (line == std::string_view::npos) return {};
			return strip(message.substr(line + 1));
		}

		static bool is_title_char(char c) {
			return std::isalnum(static_cast<unsigned char>(c)) || c == '.' ||
			       c == '_';
		}

		iterator sanitized_subject(iterator out, std::string_view message) {
			int space = 2;
			bool seen_dot = false;
			for (auto c : message) {
				if (c == '.' && seen_dot) continue;
				if (is_title_char(c)) {
					if (space == 1) *out++ = '-';
					space = 0;
					*out++ = c;
				} else
					space |= 1;
				seen_dot = c == '.';
			}
			return out;
		}

		iterator format_hash(iterator out,
		                     git_oid const* id,
		                     unsigned length = GIT_OID_HEXSZ) {
			char str[GIT_OID_HEXSZ + 1];
			git_oid_nfmt(str, length, id);
			str[length] = 0;
			return format_str(out, std::string_view{str, length});
		}

		iterator format_percentage(iterator out,
		                           io::v1::coverage_stats const& stats) {
			return fmt::format_to(out, "{:>2}%", std::get<0>(stats.calc(0)));
		}

		iterator format_rating(iterator out,
		                       io::v1::coverage_stats const& stats,
		                       context const& ctx) {
			auto mark = [&] {
				if (!stats.relevant || !ctx.marks.incomplete.den ||
				    !ctx.marks.passing.den)
					return translatable::mark_failing;
				auto const gcd_1 = std::gcd(stats.relevant, stats.covered);
				auto const cov = stats.covered / gcd_1;
				auto const rel = stats.relevant / gcd_1;

				auto const incomplete = ctx.marks.incomplete.gcd();
				auto const lhs = cov * incomplete.den;
				auto const rhs = incomplete.num * rel;
				// if ((cov * incomplete.den) < (incomplete.num * rel))
				if (lhs < rhs) return translatable::mark_failing;

				auto const passing = ctx.marks.passing.gcd();
				if ((cov * passing.den) < (passing.num * rel))
					return translatable::mark_incomplete;
				return translatable::mark_passing;
			}();
			return format_str(out, ctx.translate(0, mark, ctx.app));
		}

		iterator format_num(iterator out, auto value) {
			return fmt::format_to(out, "{}", value);
		}

		iterator format_date(iterator out, git_time_t date, char const* fmt) {
			char str[200];
			time_t in = date;
			tm local = *std::localtime(&in);
			auto const size = std::strftime(str, sizeof(str), fmt, &local);
			return format_str(out, {str, size});
		}

		using days = std::chrono::duration<
		    std::chrono::hours::rep,
		    std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>>;

		constexpr days operator""_d(unsigned long long val) {
			return days{val};
		}

		std::string relative_date(std::chrono::seconds then,
		                          context const& ctx) {
			using namespace std::chrono;
			auto const now = seconds{ctx.now};
			if (now < then)
				return ctx.translate(0, translatable::in_the_future, ctx.app);

			auto const secs = now - then;
			if (secs < 90s)
				return ctx.translate(secs.count(), translatable::seconds_ago,
				                     ctx.app);

			auto const mins = duration_cast<minutes>(secs + 30s);
			if (mins < 90min)
				return ctx.translate(mins.count(), translatable::minutes_ago,
				                     ctx.app);

			auto const hrs = duration_cast<hours>(mins + 30min);
			if (hrs < 36h)
				return ctx.translate(hrs.count(), translatable::hours_ago,
				                     ctx.app);

			auto const days = duration_cast<cov::placeholder::days>(hrs + 12h);
			if (days < 14_d)
				return ctx.translate(days.count(), translatable::days_ago,
				                     ctx.app);
			if (days < 70_d)
				return ctx.translate((days + 3_d) / 7_d,
				                     translatable::weeks_ago, ctx.app);
			if (days < 365_d)
				return ctx.translate((days + 15_d) / 30_d,
				                     translatable::months_ago, ctx.app);

			if (days < 1825_d) {
				auto const totalmonths = (days * 12 * 2 + 365_d) / (365_d * 2);
				auto const years = totalmonths / 12;
				auto const months = totalmonths % 12;
				if (months) {
					auto yrs =
					    ctx.translate(years, translatable::years_months_ago,
					                  ctx.app)
					        .append(ctx.translate(
					            months, translatable::months_ago, ctx.app));
					return yrs;
				} else
					return ctx.translate(years, translatable::years_ago,
					                     ctx.app);
			}
			return ctx.translate((days + 183_d) / 365_d,
			                     translatable::years_ago, ctx.app);
		}

		struct visitor {
			report_view const& view;
			iterator& out;
			context& ctx;

			iterator operator()(char c) {
				*out++ = c;
				return out;
			}

			iterator operator()(std::string const& s) {
				return format_str(out, s);
			}

			iterator operator()(person_info const& pair) {
				return view.format(out, ctx, pair);
			}
			iterator operator()(auto fld) { return view.format(out, ctx, fld); }
			iterator operator()(width const&) { return out; }
			iterator operator()(color) { return out; }
		};
		std::string default_tr(long long count,
		                       std::string_view singular,
		                       std::string_view plural) {
			if (count == 1) return fmt::format(fmt::runtime(singular), count);
			return fmt::format(fmt::runtime(plural), count);
		}

		std::string default_relative_date(long long count,
		                                  translatable scale,
		                                  void*) {
			switch (scale) {
				case translatable::in_the_future:
					return "in the future"s;
				case translatable::seconds_ago:
					return default_tr(count, "one second ago"sv,
					                  "{} seconds ago"sv);
				case translatable::minutes_ago:
					return default_tr(count, "one minute ago"sv,
					                  "{} minutes ago"sv);
				case translatable::hours_ago:
					return default_tr(count, "one hour ago"sv,
					                  "{} hours ago"sv);
				case translatable::days_ago:
					return default_tr(count, "one day ago"sv, "{} days ago"sv);
				case translatable::weeks_ago:
					return default_tr(count, "one week ago"sv,
					                  "{} weeks ago"sv);
				case translatable::months_ago:
					return default_tr(count, "one month ago"sv,
					                  "{} months ago"sv);
				case translatable::years_months_ago:
					return default_tr(count, "one years, "sv, "{} years, "sv);
				case translatable::years_ago:
					return default_tr(count, "one year ago"sv,
					                  "{} years ago"sv);
				case translatable::mark_failing:
					return "fail";
				case translatable::mark_incomplete:
					return "incomplete";
				case translatable::mark_passing:
					return "pass";
			}
			return std::to_string(count);
		}
	}  // namespace

	iterator refs::format(iterator out, git_oid const* id, bool wrapped) const {
		if (!id) return out;

		char buffer[GIT_OID_HEXSZ + 1];
		buffer[GIT_OID_HEXSZ] = 0;
		git_oid_fmt(buffer, id);
		auto const hash = std::string_view{buffer, GIT_OID_HEXSZ};

		bool first = true;
		if (HEAD_ref == hash) {
			first = false;
			if (wrapped) out = format_str(out, " (");
			if (HEAD.empty()) {
				out = format_str(out, "HEAD"sv);
			} else {
				out = format_str(out, "HEAD -> "sv);
				out = format_str(out, HEAD);
			}
		}

		for (auto const& [key, value] : tags) {
			if (value != hash) continue;
			if (first) {
				first = false;
				if (wrapped) out = format_str(out, " (");
			} else
				out = format_str(out, ", "sv);
			out = format_str(out, "tag: "sv);
			out = format_str(out, key);
		}

		for (auto const& [key, value] : heads) {
			if (value != hash) continue;
			if (first) {
				first = false;
				if (wrapped) out = format_str(out, " (");
			} else
				out = format_str(out, ", "sv);
			out = format_str(out, key);
		}

		if (!first && wrapped) *out++ = ')';

		return out;
	}

	iterator git_person::format(iterator out, context& ctx, person fld) const {
		switch (fld) {
			case person::name:
				return format_str(out, name);
			case person::email:
				return format_str(out, email);
			case person::email_local:
				return format_str(out, email.substr(0, email.find('@')));
			case person::date:
				return format_date(out, date, "%c");
			case person::date_relative:
				return format_str(
				    out, relative_date(std::chrono::seconds{date}, ctx));
			case person::date_timestamp:
				return format_num(out, date);
			case person::date_iso_like:
				return format_date(out, date, "%F %T %z");
			case person::date_iso_strict:
				return format_date(out, date, "%FT%T%z");
			case person::date_short:
				return format_date(out, date, "%F");
		}
		return out;
	}

	iterator git_commit_view::format(iterator out,
	                                 context& ctx,
	                                 commit fld) const {
		switch (fld) {
			case commit::hash:
				return format_hash(out, id);
			case commit::hash_abbr:
				return format_hash(out, id, ctx.hash_length);
			case commit::subject:
				return format_str(out, subject_from(message));
			case commit::subject_sanitized:
				return sanitized_subject(out, subject_from(message));
			case commit::body:
				return format_str(out, body_from(message));
			case commit::body_raw:
				return format_str(out, message);
		}
		return out;
	}

	iterator report_view::format(iterator out, context& ctx, report fld) const {
		switch (fld) {
			case report::hash:
				return format_hash(out, id);
			case report::hash_abbr:
				return format_hash(out, id, ctx.hash_length);
			case report::parent_hash:
				return format_hash(out, parent);
			case report::parent_hash_abbr:
				return format_hash(out, parent, ctx.hash_length);
			case report::ref_names:
				return ctx.names.format(out, id, true);
			case report::ref_names_unwrapped:
				return ctx.names.format(out, id, false);
			case report::branch:
				return format_str(out, git.branch);
			case report::lines_percent:
				return !stats ? out : format_percentage(out, *stats);
			case report::lines_total:
				return !stats ? out : format_num(out, stats->total);
			case report::lines_relevant:
				return !stats ? out : format_num(out, stats->relevant);
			case report::lines_covered:
				return !stats ? out : format_num(out, stats->covered);
			case report::lines_rating:
				return !stats ? out : format_rating(out, *stats, ctx);
		}
		return out;
	}

	iterator report_view::format_with(iterator out,
	                                  context& ctx,
	                                  placeholder::format const& fmt) const {
		return std::visit(visitor{*this, out, ctx}, fmt);
	}
}  // namespace cov::placeholder

namespace cov {
	namespace parser {
		using placeholder::format;

#define SIMPLEST_FORMAT(CHAR, RESULT) \
	case CHAR:                        \
		++cur;                        \
		return RESULT
#define SIMPLE_FORMAT(CHAR, RESULT) \
	case CHAR:                      \
		++cur;                      \
		return format { RESULT }
#define DEEPER_FORMAT(CHAR, CB) \
	case CHAR:                  \
		++cur;                  \
		return CB(cur, end)
#define DEEPER_FORMAT1(CHAR, CB, ARG) \
	case CHAR:                        \
		++cur;                        \
		return CB(cur, end, ARG)
		unsigned hex(char c) {
			switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					return static_cast<unsigned>(c - '0');
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					return static_cast<unsigned>(c - 'A' + 10);
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					return static_cast<unsigned>(c - 'a' + 10);
			}
			return 16;
		}

		template <typename It>
		std::optional<placeholder::format> parse_hex(It& cur, It end) {
			if (cur == end) return std::nullopt;
			auto const hi_nybble = hex(*cur);
			if (hi_nybble > 15) return std::nullopt;
			++cur;
			if (cur == end) return std::nullopt;
			auto const lo_nybble = hex(*cur);
			if (lo_nybble > 15) return std::nullopt;
			++cur;

			return format{static_cast<char>(lo_nybble | (hi_nybble << 4))};
		}
		template <typename It>
		std::optional<placeholder::format> parse_hash(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('r', placeholder::report::hash);
				SIMPLE_FORMAT('c', placeholder::commit::hash);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::format> parse_hash_abbr(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('r', placeholder::report::hash_abbr);
				SIMPLE_FORMAT('c', placeholder::commit::hash_abbr);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::person> parse_date(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLEST_FORMAT('d', placeholder::person::date);
				SIMPLEST_FORMAT('r', placeholder::person::date_relative);
				SIMPLEST_FORMAT('t', placeholder::person::date_timestamp);
				SIMPLEST_FORMAT('i', placeholder::person::date_iso_like);
				SIMPLEST_FORMAT('I', placeholder::person::date_iso_strict);
				SIMPLEST_FORMAT('s', placeholder::person::date_short);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::format> parse_report(It& cur, It end) {
			using placeholder::person_info;
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('P', placeholder::report::parent_hash);
				SIMPLE_FORMAT('p', placeholder::report::parent_hash_abbr);
				SIMPLE_FORMAT('D', placeholder::report::branch);
				default: {
					auto const person = parse_date(cur, end);
					if (!person) break;
					return format{
					    person_info{placeholder::who::reporter, *person}};
				}
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::format> parse_stats(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('P', placeholder::report::lines_percent);
				SIMPLE_FORMAT('T', placeholder::report::lines_total);
				SIMPLE_FORMAT('R', placeholder::report::lines_relevant);
				SIMPLE_FORMAT('C', placeholder::report::lines_covered);
				SIMPLE_FORMAT('r', placeholder::report::lines_rating);
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::format> parse_person(It& cur,
		                                                It end,
		                                                placeholder::who who) {
			if (cur == end) return std::nullopt;

			using placeholder::person;
			using placeholder::person_info;
			// person_info{who, person::}

			switch (*cur) {
				SIMPLE_FORMAT('n', (person_info{who, person::name}));
				SIMPLE_FORMAT('e', (person_info{who, person::email}));
				SIMPLE_FORMAT('l', (person_info{who, person::email_local}));
				default: {
					auto const person = parse_date(cur, end);
					if (!person) break;
					return format{person_info{who, *person}};
				}
			}
			return std::nullopt;
		}
		template <typename It>
		std::optional<placeholder::format> parse(It& cur, It end) {
			if (cur == end) return std::nullopt;

			switch (*cur) {
				SIMPLE_FORMAT('%', '%');
				SIMPLE_FORMAT('n', '\n');
				DEEPER_FORMAT('x', parse_hex);
				SIMPLE_FORMAT('D', placeholder::report::ref_names_unwrapped);
				SIMPLE_FORMAT('d', placeholder::report::ref_names);
				SIMPLE_FORMAT('s', placeholder::commit::subject);
				SIMPLE_FORMAT('f', placeholder::commit::subject_sanitized);
				SIMPLE_FORMAT('b', placeholder::commit::body);
				SIMPLE_FORMAT('B', placeholder::commit::body_raw);
				DEEPER_FORMAT('H', parse_hash);
				DEEPER_FORMAT('h', parse_hash_abbr);
				DEEPER_FORMAT('r', parse_report);
				DEEPER_FORMAT('p', parse_stats);
				DEEPER_FORMAT1('a', parse_person, placeholder::who::author);
				DEEPER_FORMAT1('c', parse_person, placeholder::who::committer);
			}
			return std::nullopt;
		}
	}  // namespace parser

	formatter formatter::from(std::string_view input) {
		std::vector<placeholder::format> fmts{};
		auto cur = input.begin();
		auto end = input.end();
		auto prev = cur;
		while (cur != end) {
			while (cur != end && *cur != '%')
				++cur;
			if (cur == end) break;
			auto text_end = cur;
			++cur;
			auto arg = parser::parse(cur, end);
			if (!arg) continue;
			if (prev != text_end) fmts.push_back(std::string{prev, text_end});
			fmts.push_back(std::move(*arg));
			prev = cur;
		}
		if (prev != end) fmts.push_back(std::string{prev, end});
		return formatter{std::move(fmts)};
	}

	std::string formatter::format(placeholder::report_view const& report,
	                              placeholder::context& ctx) {
		std::string result{};
		if (!ctx.translate) ctx.translate = placeholder::default_relative_date;

		for (auto const& fmt : format_)
			report.format_with(std::back_inserter(result), ctx, fmt);

		return result;
	}
}  // namespace cov
