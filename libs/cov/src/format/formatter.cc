// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4189)
#endif
#include <fmt/format.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <date/tz.h>
#include <fmt/chrono.h>
#include <charconv>
#include <cov/format.hh>
#include <ctime>
#include <iostream>
#include "../path-utils.hh"

namespace cov::placeholder {
	struct internal_context {
		context const* client;
		void* app;
		std::string (*tr)(long long count, translatable scale, void* app);
		date::time_zone const* tz;
		iterator formatted_output(iterator out, std::string_view view);
		std::optional<width> current_witdh{};

		std::string translate(long long count, translatable scale) const {
			return tr(count, scale, app);
		}

		std::string translate(translatable scale) const {
			return tr(0, scale, app);
		}
	};

	namespace {
		struct leaky_iterator : iterator {
			auto cont() { return container; }
		};

		iterator format_str(iterator out, std::string_view view) {
			leaky_iterator{out}.cont()->append(view);
			return out;
		}
	}  // namespace

	iterator internal_context::formatted_output(iterator out,
	                                            std::string_view view) {
		if (!current_witdh) return format_str(out, view);
		view = strip(view);
		auto first = true;
		unsigned indent = current_witdh->indent1;
		while (!view.empty()) {
			if (first)
				first = false;
			else
				out = format_str(out, "\n\n"sv);

			auto const pos = view.find("\n\n"sv);
			auto chunk = view.substr(0, pos);

			size_t index{}, length{current_witdh->total - indent};
			auto const indent_str = std::string(indent, ' ');
			while (!chunk.empty()) {
				size_t word_len{};
				while (word_len < chunk.length() && !isspace(chunk[word_len]))
					++word_len;
				auto const word = chunk.substr(0, word_len);
				while (word_len < chunk.length() && isspace(chunk[word_len]))
					++word_len;
				chunk = chunk.substr(word_len);

				if (index && (index + word.length() + 1) > length) {
					*out++ = '\n';
					index = 0;
				} else if (index) {
					index++;
					*out++ = ' ';
				}

				if (!index) out = format_str(out, indent_str);
				out = format_str(out, word);
				index += word.length();
			}
			view = pos == std::string_view::npos ? std::string_view{}
			                                     : lstrip(view.substr(pos));
			indent = current_witdh->indent2;
		}

		return out;
	}

	namespace {
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
				} else {
					space |= 1;
				}
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
			return fmt::format_to(out, "{:>3}%", stats.calc(0).whole);
		}

		static auto apply_mark(io::v1::coverage_stats const& stats,
		                       rating const& marks) {
			if (!stats.relevant || !marks.incomplete.den || !marks.passing.den)
				return translatable::mark_failing;
			auto const gcd_1 = std::gcd(stats.relevant, stats.covered);
			auto const cov = stats.covered / gcd_1;
			auto const rel = stats.relevant / gcd_1;

			auto const incomplete = marks.incomplete.gcd();
			auto const lhs = cov * incomplete.den;
			auto const rhs = incomplete.num * rel;
			// if ((cov * incomplete.den) < (incomplete.num * rel))
			if (lhs < rhs) return translatable::mark_failing;

			auto const passing = marks.passing.gcd();
			if ((cov * passing.den) < (passing.num * rel))
				return translatable::mark_incomplete;
			return translatable::mark_passing;
		}

		iterator format_rating(iterator out,
		                       io::v1::coverage_stats const& stats,
		                       internal_context const& ctx) {
			auto mark = apply_mark(stats, ctx.client->marks);
			return format_str(out, ctx.translate(mark));
		}

		iterator format_num(iterator out, auto value) {
			return fmt::format_to(out, "{}", value);
		}

		enum class Z { none, ISO, ISO_colon };

		template <typename Storage, typename Ratio>
		static int to_int(std::chrono::duration<Storage, Ratio> dur) {
			return static_cast<int>(dur.count());
		}

		static int to_uint(auto dur) {
			return static_cast<int>(static_cast<unsigned int>(dur));
		}

#ifdef __struct_tm_defined  // Linux?
#define HAS_TM_GMTOFF
#endif

		static
#ifdef HAS_TM_GMTOFF
		    std::pair<tm, std::string>
#else
		    tm
#endif
		    sys_to_tm(sys_seconds date, ::date::time_zone const* tz) {
			auto const local_seconds = tz->to_local(date);
			auto const faux_sys = sys_seconds{local_seconds.time_since_epoch()};
			auto const days = std::chrono::floor<date::days>(faux_sys);
			auto const sys_date = date::year_month_day{days};
			auto const weekday = date::year_month_weekday{days}.weekday();
			auto const sys_hms = date::hh_mm_ss{faux_sys - days};

			return {
#ifdef HAS_TM_GMTOFF
			    {
#endif
			        .tm_sec = to_int(sys_hms.seconds()),
			        .tm_min = to_int(sys_hms.minutes()),
			        .tm_hour = to_int(sys_hms.hours()),
			        .tm_mday = to_uint(sys_date.day()),
			        .tm_mon = to_uint(sys_date.month()) - 1,
			        .tm_year = static_cast<int>(sys_date.year()) - 1900,
			        .tm_wday = to_uint(weekday.c_encoding()),
			        .tm_yday = -1,
			        .tm_isdst = -1,
#ifdef HAS_TM_GMTOFF
			        .tm_gmtoff = tz->get_info(date).offset.count(),
			        .tm_zone = nullptr,
			    },
			    tz->get_info(date).abbrev,
#endif
			};
		}

		iterator format_date(iterator out,
		                     sys_seconds date,
		                     ::date::time_zone const* tz,
		                     std::string_view locale,
		                     std::string_view fmt,
		                     Z suffix = Z::none) {
			auto loc =
			    locale.empty()
			        ? std::locale{""}
			        : std::locale{std::string{locale.data(), locale.size()}};

#ifdef HAS_TM_GMTOFF
			auto [tm, abbrev] = sys_to_tm(date, tz);
			tm.tm_zone = abbrev.c_str();
#else
			auto const tm = sys_to_tm(date, tz);
#endif

			out = fmt::format_to(out, loc, fmt::runtime(fmt), tm);

			if (suffix != Z::none) {
				auto const offset = tz->get_info(date).offset;
				auto const hours =
				    std::chrono::floor<std::chrono::hours>(offset);
				auto const minutes =
				    std::chrono::floor<std::chrono::minutes>(offset - hours);

				if (suffix == Z::ISO)
					out = fmt::format_to(out, "{:+03}{:02}"sv, hours.count(),
					                     minutes.count());
				else
					out = fmt::format_to(out, "{:+03}:{:02}"sv, hours.count(),
					                     minutes.count());
			}

			return out;
		}

		using days = std::chrono::duration<
		    std::chrono::hours::rep,
		    std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>>;

		constexpr days operator""_d(unsigned long long val) {
			return days{val};
		}

		std::string relative_date(sys_seconds then,
		                          internal_context const& ctx) {
			using namespace std::chrono;
			if (ctx.client->now < then)
				return ctx.translate(translatable::in_the_future);

			auto const secs = ctx.client->now - then;
			if (secs < 90s)
				return ctx.translate(secs.count(), translatable::seconds_ago);

			auto const mins = duration_cast<minutes>(secs + 30s);
			if (mins < 90min)
				return ctx.translate(mins.count(), translatable::minutes_ago);

			auto const hrs = duration_cast<hours>(mins + 30min);
			if (hrs < 36h)
				return ctx.translate(hrs.count(), translatable::hours_ago);

			auto const days = duration_cast<cov::placeholder::days>(hrs + 12h);
			if (days < 14_d)
				return ctx.translate(days.count(), translatable::days_ago);
			if (days < 70_d)
				return ctx.translate((days + 3_d) / 7_d,
				                     translatable::weeks_ago);
			if (days < 365_d)
				return ctx.translate((days + 15_d) / 30_d,
				                     translatable::months_ago);

			if (days < 1825_d) {
				auto const totalmonths = (days * 12 * 2 + 365_d) / (365_d * 2);
				auto const years = totalmonths / 12;
				auto const months = totalmonths % 12;
				if (months) {
					auto yrs =
					    ctx.translate(years, translatable::years_months_ago)
					        .append(" ")
					        .append(ctx.translate(months,
					                              translatable::months_ago));
					return yrs;
				} else {  // GCOV_EXCL_LINE[WIN32]
					return ctx.translate(years, translatable::years_ago);
				}
			}
			return ctx.translate((days + 183_d) / 365_d,
			                     translatable::years_ago);
		}

		struct visitor {
			report_view const& view;
			iterator out;
			internal_context& ctx;

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
			iterator operator()(width const& w) {
				ctx.current_witdh = w;
				return out;
			}
		};
	}  // namespace

	iterator refs::format(iterator out,
	                      git_oid const* id,
	                      bool wrapped,
	                      bool magic_colors,
	                      struct report_view const& view,
	                      internal_context& ctx) const {
		if (!id || !ctx.client->decorate) return out;

		char buffer[GIT_OID_HEXSZ + 1];
		buffer[GIT_OID_HEXSZ] = 0;
		git_oid_fmt(buffer, id);
		auto const hash = std::string_view{buffer, GIT_OID_HEXSZ};

		auto const color_str = [&](iterator out_iter, color clr,
		                           std::string_view str) {
			if (magic_colors) out_iter = view.format(out_iter, ctx, clr);
			out_iter = format_str(out_iter, str);
			if (magic_colors)
				out_iter = view.format(out_iter, ctx, color::reset);
			return out_iter;
		};

		bool first = true;
		if (HEAD_ref == hash) {
			first = false;
			if (wrapped) out = color_str(out, color::yellow, " ("sv);
			if (HEAD.empty()) {
				out = color_str(out, color::bold_cyan, "HEAD"sv);
			} else {
				out = color_str(out, color::bold_cyan, "HEAD -> "sv);
				out = color_str(out, color::bold_green, HEAD);
			}
		}

		for (auto const& [key, value] : tags) {
			if (value != hash) continue;
			if (first) {
				first = false;
				if (wrapped) out = color_str(out, color::yellow, " ("sv);
			} else {
				out = color_str(out, color::yellow, ", "sv);
			}
			if (magic_colors) out = view.format(out, ctx, color::bold_yellow);
			out = format_str(out, "tag: "sv);
			out = format_str(out, key);
			if (magic_colors) out = view.format(out, ctx, color::reset);
		}

		for (auto const& [key, value] : heads) {
			if (value != hash) continue;
			if (key == HEAD) continue;
			if (first) {
				first = false;
				if (wrapped) out = color_str(out, color::yellow, " ("sv);
			} else {
				out = color_str(out, color::yellow, ", "sv);
			}
			out = color_str(out, color::bold_green, key);
		}

		if (!first && wrapped) {
			if (magic_colors) {
				out = view.format(out, ctx, color::yellow);
				*out++ = ')';
				out = view.format(out, ctx, color::reset);
			} else {
				*out++ = ')';
			}
		}

		return out;
	}

	iterator git_person::format(iterator out,
	                            internal_context& ctx,
	                            person fld) const {
		switch (fld) {
			case person::name:
				return format_str(out, name);
			case person::email:
				return format_str(out, email);
			case person::email_local:
				return format_str(out, email.substr(0, email.find('@')));
			case person::date:
				return format_date(out, date, ctx.tz, ctx.client->locale,
				                   "{:%c}"sv);
			case person::date_relative:
				return format_str(out, relative_date(date, ctx));
			case person::date_timestamp:
				return format_num(out, date.time_since_epoch().count());
			case person::date_iso_like:
				return format_date(out, date, ctx.tz, ctx.client->locale,
				                   "{:%F %T }"sv, Z::ISO_colon);
			case person::date_iso_strict:
				return format_date(out, date, ctx.tz, ctx.client->locale,
				                   "{:%FT%T}"sv, Z::ISO);
			case person::date_short:
				return format_date(out, date, ctx.tz, ctx.client->locale,
				                   "{:%F}"sv);
		}
		return out;  // GCOV_EXCL_LINE - all enums are handled above
	}

	iterator git_commit_view::format(iterator out,
	                                 internal_context& ctx,
	                                 commit fld) const {
		switch (fld) {
			case commit::hash:
				return format_hash(out, id);
			case commit::hash_abbr:
				return format_hash(out, id, ctx.client->hash_length);
			case commit::subject:
				return ctx.formatted_output(out, subject_from(message));
			case commit::subject_sanitized:
				return sanitized_subject(out, subject_from(message));
			case commit::body:
				return ctx.formatted_output(out, body_from(message));
			case commit::body_raw:
				return ctx.formatted_output(out, strip(message));
		}
		return out;  // GCOV_EXCL_LINE - all enums are handled above
	}

	iterator report_view::format(iterator out,
	                             internal_context& ctx,
	                             report fld) const {
		width_cleaner clean{ctx};

		switch (fld) {
			case report::hash:
				return format_hash(out, id);
			case report::hash_abbr:
				return format_hash(out, id, ctx.client->hash_length);
			case report::parent_hash:
				return format_hash(out, parent);
			case report::parent_hash_abbr:
				return format_hash(out, parent, ctx.client->hash_length);
			case report::file_list_hash:
				return format_hash(out, file_list);
			case report::file_list_hash_abbr:
				return format_hash(out, file_list, ctx.client->hash_length);
			case report::ref_names:
				return ctx.client->names.format(out, id, true, false, *this,
				                                ctx);
			case report::ref_names_unwrapped:
				return ctx.client->names.format(out, id, false, false, *this,
				                                ctx);
			case report::magic_ref_names:
				return ctx.client->names.format(out, id, true, true, *this,
				                                ctx);
			case report::magic_ref_names_unwrapped:
				return ctx.client->names.format(out, id, false, true, *this,
				                                ctx);
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
		return out;  // GCOV_EXCL_LINE - all enums are handled above
	}

	static color mark_color(translatable mark,
	                        color pass,
	                        color incomplete,
	                        color fail) {
		return mark == cov::translatable::mark_failing      ? fail
		       : mark == cov::translatable::mark_incomplete ? incomplete
		                                                    : pass;
	}

#define MARK_COLOR(...)                                                        \
	if (clr == color::__VA_ARGS__##rating)                                     \
		clr = mark_color(mark, color::__VA_ARGS__##green,                      \
		                 color::__VA_ARGS__##yellow, color::__VA_ARGS__##red); \
	else

	iterator report_view::format(iterator out,
	                             internal_context& ctx,
	                             color clr) const {
		width_cleaner clean{ctx};
		if (ctx.client->colorize) {
			if (clr == color::rating || clr == color::bold_rating ||
			    clr == color::faint_rating || clr == color::bg_rating) {
				auto const mark = apply_mark(*stats, ctx.client->marks);
				MARK_COLOR()
				MARK_COLOR(bold_)
				MARK_COLOR(faint_)
				MARK_COLOR(bg_) {}
			}
			out = format_str(
			    out,
			    ctx.client->colorize(
			        clr, ctx.client->app));  // don't count towards width...
		}
		return out;
	}

	iterator report_view::format_with(iterator out,
	                                  internal_context& ctx,
	                                  placeholder::format const& fmt) const {
		return std::visit(visitor{*this, out, ctx}, fmt);
	}

	report_view::width_cleaner::~width_cleaner() noexcept {
		ctx.current_witdh = std::nullopt;
	}
}  // namespace cov::placeholder

namespace cov {
	std::string formatter::format(placeholder::report_view const& report,
	                              placeholder::context const& ctx) {
		std::string result{};

		placeholder::internal_context int_ctx{
		    .client = &ctx,
		    .app = ctx.app,
		    .tr = ctx.translate ? ctx.translate : no_translation,
		    .tz = needs_timezones_ ? ctx.time_zone.empty()
		                                 ? date::current_zone()
		                                 : date::locate_zone(ctx.time_zone)
		                           : nullptr,
		};

		for (auto const& fmt : format_)
			report.format_with(std::back_inserter(result), int_ctx, fmt);

		return result;
	}
}  // namespace cov
