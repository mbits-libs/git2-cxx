// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/chrono.h>
#include <charconv>
#include <cov/repository.hh>
#include <ctime>
#include <iostream>
#include "../path-utils.hh"
#include "internal_environment.hh"

namespace cov::placeholder {
	namespace {
		struct leaky_iterator : iterator {
			auto cont() { return container; }
		};

		iterator format_str(iterator out, std::string_view view) {
			leaky_iterator{out}.cont()->append(view);
			return out;
		}
	}  // namespace

	iterator internal_environment::formatted_output(iterator out,
	                                                std::string_view view) {
		if (!current_width) return format_str(out, view);
		view = strip(view);
		auto first = true;
		unsigned indent = current_width->indent1;
		while (!view.empty()) {
			if (first)
				first = false;
			else
				out = format_str(out, "\n\n"sv);

			auto const pos = view.find("\n\n"sv);
			auto chunk = view.substr(0, pos);

			size_t index{}, length{current_width->total - indent};
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
			indent = current_width->indent2;
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
		                     git::oid const* id,
		                     unsigned length = GIT_OID_HEXSZ) {
			return format_str(out, id->str(length));
		}

		iterator format_percentage(iterator out, io::v1::stats const& stats) {
			return fmt::format_to(out, "{:>3}%", stats.calc(0).whole);
		}

		iterator format_rating(iterator out,
		                       io::v1::stats const& stats,
		                       internal_environment const& env) {
			auto mark = formatter::apply_mark(stats, env.client->marks);
			return format_str(out, env.translate(mark));
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
		                          internal_environment const& env) {
			using namespace std::chrono;
			if (env.client->now < then)
				return env.translate(translatable::in_the_future);

			auto const secs = env.client->now - then;
			if (secs < 90s)
				return env.translate(secs.count(), translatable::seconds_ago);

			auto const mins = duration_cast<minutes>(secs + 30s);
			if (mins < 90min)
				return env.translate(mins.count(), translatable::minutes_ago);

			auto const hrs = duration_cast<hours>(mins + 30min);
			if (hrs < 36h)
				return env.translate(hrs.count(), translatable::hours_ago);

			auto const days = duration_cast<cov::placeholder::days>(hrs + 12h);
			if (days < 14_d)
				return env.translate(days.count(), translatable::days_ago);
			if (days < 70_d)
				return env.translate((days + 3_d) / 7_d,
				                     translatable::weeks_ago);
			if (days < 365_d)
				return env.translate((days + 15_d) / 30_d,
				                     translatable::months_ago);

			if (days < 1825_d) {
				auto const totalmonths = (days * 12 * 2 + 365_d) / (365_d * 2);
				auto const years = totalmonths / 12;
				auto const months = totalmonths % 12;
				if (months) {
					auto yrs =
					    env.translate(years, translatable::years_months_ago)
					        .append(" ")
					        .append(env.translate(months,
					                              translatable::months_ago));
					return yrs;
				} else {  // GCOV_EXCL_LINE[WIN32]
					return env.translate(years, translatable::years_ago);
				}
			}
			return env.translate((days + 183_d) / 365_d,
			                     translatable::years_ago);
		}

		struct context_visitor {
			context const& ctx;
			iterator out;
			internal_environment& env;

			iterator operator()(char c) {
				*out++ = c;
				return out;
			}

			iterator operator()(std::string const& s) {
				return format_str(out, s);
			}

			iterator operator()(person_info const& pair) {
				return ctx.format(out, env, pair);
			}
			iterator operator()(auto fld) { return ctx.format(out, env, fld); }
			iterator operator()(width const& w) {
				env.current_width = w;
				return out;
			}

			iterator operator()(block const& b) {
				if (!ctx.facade) return out;

				if (b.type == block_type::if_start) {
					if (!ctx.facade->condition(b.ref)) {
						return out;
					}
					return ctx.format_all(out, env, b.opcodes);
				}

				// fmt::print(stderr, "entering facade loop for {}\n", b.ref);

				auto iter = ctx.facade->loop(b.ref);

				if (iter) {
					do {
						auto const next = iter->next();
						if (!next) break;
						out =
						    context{next.get()}.format_all(out, env, b.opcodes);
					} while (true);
				}

				return out;
			}
		};

		struct property_visitor {
			iterator out;
			bool magic_colors;
			context const& ctx;
			internal_environment& env;
			std::string const* key{};
			std::string_view prefix{};

			iterator color_str(iterator out_iter,
			                   color clr,
			                   std::string_view str) const {
				if (magic_colors) out_iter = ctx.format(out_iter, env, clr);
				out_iter = format_str(out_iter, str);
				if (magic_colors)
					out_iter = ctx.format(out_iter, env, color::reset);
				return out_iter;
			}

			void color_str(color clr, std::string_view str) {
				out = color_str(out, clr, str);
			}

			void color_key(bool force, bool colon = true) {
				if (force || env.client->prop_names) {
					if (colon) {
						color_str(color::faint_yellow,
						          fmt::format("{}{}: ", prefix, *key));

					} else {
						color_str(color::faint_yellow,
						          fmt::format("{}{}", prefix, *key));
					}
				} else if (!prefix.empty()) {
					color_str(color::faint_yellow, prefix);
				}
			}

			void operator()(std::string_view str) {
				if (str.empty()) {
					color_key(true, false);
					return;
				}
				color_key(false);
				color_str(color::yellow, str);
			}

			void operator()(long long val) {
				color_key(true);
				color_str(color::green, fmt::format("{}", val));
			}

			void operator()(bool val) {
				color_key(true);
				color_str(color::blue, val ? "on"sv : "off"sv);
			}
		};
	}  // namespace

	iterator refs::format(iterator out,
	                      git::oid const* id,
	                      bool wrapped,
	                      bool magic_colors,
	                      context const& ctx,
	                      internal_environment& env) const {
		if (!id || !env.client->decorate) return out;

		auto const hash = id->str();
		auto const color_str = [&](iterator out_iter, color clr,
		                           std::string_view str) {
			if (magic_colors) out_iter = ctx.format(out_iter, env, clr);
			out_iter = format_str(out_iter, str);
			if (magic_colors)
				out_iter = ctx.format(out_iter, env, color::reset);
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
			if (magic_colors) out = ctx.format(out, env, color::bold_yellow);
			out = format_str(out, "tag: "sv);
			out = format_str(out, key);
			if (magic_colors) out = ctx.format(out, env, color::reset);
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
				out = ctx.format(out, env, color::yellow);
				*out++ = ')';
				out = ctx.format(out, env, color::reset);
			} else {
				*out++ = ')';
			}
		}

		return out;
	}

	iterator git_person::format(iterator out,
	                            internal_environment& env,
	                            person fld) const {
		switch (fld) {
			case person::name:
				return format_str(out, name);
			case person::email:
				return format_str(out, email);
			case person::email_local:
				return format_str(out, email.substr(0, email.find('@')));
			case person::date:
				return format_date(out, date, env.tz, env.client->locale,
				                   "{:%c}"sv);
			case person::date_relative:
				return format_str(out, relative_date(date, env));
			case person::date_timestamp:
				return format_num(out, date.time_since_epoch().count());
			case person::date_iso_like:
				return format_date(out, date, env.tz, env.client->locale,
				                   "{:%F %T }"sv, Z::ISO_colon);
			case person::date_iso_strict:
				return format_date(out, date, env.tz, env.client->locale,
				                   "{:%FT%T}"sv, Z::ISO);
			case person::date_short:
				return format_date(out, date, env.tz, env.client->locale,
				                   "{:%F}"sv);
		}
		return out;  // GCOV_EXCL_LINE - all enums are handled above
	}

	iterator git_commit_view::format(iterator out,
	                                 internal_environment& env,
	                                 commit fld) const {
		switch (fld) {
			case commit::subject:
				return env.formatted_output(out, subject_from(message));
			case commit::subject_sanitized:
				return sanitized_subject(out, subject_from(message));
			case commit::body:
				return env.formatted_output(out, body_from(message));
			case commit::body_raw:
				return env.formatted_output(out, strip(message));
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

	iterator context::format(iterator out,
	                         internal_environment& env,
	                         color clr) const {
		width_cleaner clean{env};
		if (env.client->colorize) {
			if (clr == color::rating || clr == color::bold_rating ||
			    clr == color::faint_rating || clr == color::bg_rating) {
				io::v1::coverage_stats const* stats =
				    facade ? facade->stats() : nullptr;
				clr = formatter::apply_mark(
				    clr, stats ? stats->lines : io::v1::stats{0, 0},
				    env.client->marks);
			}
			out = format_str(
			    out,
			    env.client->colorize(
			        clr, env.client->app));  // don't count towards width...
		}
		return out;
	}

	iterator context::format(iterator out,
	                         internal_environment& env,
	                         self fld) const {
		width_cleaner clean{env};
		if (!facade) return out;

		git::oid const* id{};
		bool abbr{false};

		switch (fld) {
			case self::primary_hash_abbr:
				abbr = true;
				[[fallthrough]];
			case self::primary_hash:
				id = facade->primary_id();
				break;

			case self::secondary_hash_abbr:
				abbr = true;
				[[fallthrough]];
			case self::secondary_hash:
				id = facade->secondary_id();
				break;

			case self::tertiary_hash_abbr:
				abbr = true;
				[[fallthrough]];
			case self::tertiary_hash:
				id = facade->tertiary_id();
				break;

			case self::quaternary_hash_abbr:
				abbr = true;
				[[fallthrough]];
			case self::quaternary_hash:
				id = facade->quaternary_id();
				break;
		}

		if (!id) {
			return out;
		}

		if (abbr) {
			return format_hash(out, id, env.client->hash_length);
		}

		return format_hash(out, id);
	}

	iterator context::format(iterator out,
	                         internal_environment& env,
	                         placeholder::stats fld) const {
		width_cleaner clean{env};
		io::v1::coverage_stats const* stats =
		    facade ? facade->stats() : nullptr;

		switch (fld) {
			case stats::lines:
				return !stats ? out : format_num(out, stats->lines_total);
			case stats::lines_total:
				return !stats ? out : format_num(out, stats->lines.relevant);
			case stats::lines_visited:
				return !stats ? out : format_num(out, stats->lines.visited);
			case stats::lines_rating:
				return !stats ? out : format_rating(out, stats->lines, env);
			case stats::lines_percent:
				return !stats ? out : format_percentage(out, stats->lines);
			case stats::functions_total:
				return !stats ? out
				              : format_num(out, stats->functions.relevant);
			case stats::functions_visited:
				return !stats ? out : format_num(out, stats->functions.visited);
			case stats::functions_rating:
				return !stats ? out : format_rating(out, stats->functions, env);
			case stats::functions_percent:
				return !stats ? out : format_percentage(out, stats->functions);
			case stats::branches_total:
				return !stats ? out : format_num(out, stats->branches.relevant);
			case stats::branches_visited:
				return !stats ? out : format_num(out, stats->branches.visited);
			case stats::branches_rating:
				return !stats ? out : format_rating(out, stats->branches, env);
			case stats::branches_percent:
				return !stats ? out : format_percentage(out, stats->branches);
		}
		return out;  // GCOV_EXCL_LINE - all enums are handled above
	}

	iterator context::format(iterator out,
	                         internal_environment& env,
	                         report fld) const {
		width_cleaner clean{env};

		switch (fld) {
			case report::ref_names:
				return facade ? facade->prop(out, true, false, env) : out;
			case report::ref_names_unwrapped:
				return facade ? facade->prop(out, false, false, env) : out;
			case report::magic_ref_names:
				return facade ? facade->prop(out, true, true, env) : out;
			case report::magic_ref_names_unwrapped:
				return facade ? facade->prop(out, false, true, env) : out;
			case report::branch: {
				git_commit_view const* git = facade ? facade->git() : nullptr;
				return git ? format_str(out, git->branch) : out;
			}
		}
		return out;  // GCOV_EXCL_LINE - all enums are handled above
	}

	iterator context::format(iterator out,
	                         internal_environment& env,
	                         commit fld) const {
		git_commit_view const* git = facade ? facade->git() : nullptr;
		return git ? git->format(out, env, fld) : out;
	}

	iterator context::format(iterator out,
	                         internal_environment& env,
	                         person_info const& pair) const {
		auto const [who, person] = pair;
		if (who == placeholder::who::reporter) {
			return facade ? git_person{{}, {}, facade->added()}.format(out, env,
			                                                           person)
			              : out;
		}

		git_commit_view const* git = facade ? facade->git() : nullptr;
		return git ? git->format(out, env, pair) : out;
	}

	iterator context::format_with(iterator out,
	                              internal_environment& env,
	                              placeholder::printable const& op) const {
		return std::visit(context_visitor{*this, out, env}, op);
	}

	iterator context::format_all(
	    iterator out,
	    internal_environment& env,
	    std::vector<placeholder::printable> const& opcodes) const {
		for (auto const& op : opcodes)
			out = format_with(out, env, op);
		return out;
	}

	iterator context::format_properties(
	    iterator out,
	    internal_environment& env,
	    bool wrapped,
	    bool magic_colors,
	    std::map<std::string, cov::report::property> const& properties) const {
		if (properties.empty() || !env.client->decorate) return out;

		property_visitor painter{out, magic_colors, *this, env};

		bool first = true;
		for (auto const& [key, value] : properties) {
			painter.key = &key;
			painter.prefix = {};
			if (first) {
				first = false;
				if (wrapped) painter.prefix = " ("sv;
			} else {
				painter.prefix = ", "sv;
			}
			std::visit(painter, value);
		}
		out = painter.out;

		if (!first && wrapped) {
			if (magic_colors) {
				out = format(out, env, color::faint_yellow);
				*out++ = ')';
				out = format(out, env, color::reset);
			} else {
				*out++ = ')';
			}
		}

		return out;
	}

	context::width_cleaner::~width_cleaner() noexcept {
		env.current_width = std::nullopt;
	}

}  // namespace cov::placeholder

namespace cov {
	std::string formatter::format(placeholder::context const& ctx,
	                              placeholder::environment const& env) const {
		std::string result{};

		placeholder::internal_environment int_ctx{
		    .client = &env,
		    .app = env.app,
		    .tr = env.translate ? env.translate : no_translation,
		    .tz = needs_timezones_ ? env.time_zone.empty()
		                                 ? date::current_zone()
		                                 : date::locate_zone(env.time_zone)
		                           : nullptr,
		};

		ctx.format_all(std::back_inserter(result), int_ctx, format_);
		return result;
	}

	translatable formatter::apply_mark(io::v1::stats const& stats,
	                                   placeholder::rating const& marks) {
		if (!stats.relevant || !marks.incomplete.den || !marks.passing.den)
			return translatable::mark_failing;
		auto const gcd_1 = std::gcd(stats.relevant, stats.visited);
		auto const cov = stats.visited / gcd_1;
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

#define MARK_COLOR(...)                                    \
	case color::__VA_ARGS__##rating:                       \
		return mark_color(mark, color::__VA_ARGS__##green, \
		                  color::__VA_ARGS__##yellow,      \
		                  color::__VA_ARGS__##red);

	placeholder::color formatter::apply_mark(placeholder::color clr,
	                                         io::v1::stats const& stats,
	                                         placeholder::rating const& marks) {
		using placeholder::color;
		if (clr == color::rating || clr == color::bold_rating ||
		    clr == color::faint_rating || clr == color::bg_rating) {
			auto const mark = apply_mark(stats, marks);
			switch (clr) {
				MARK_COLOR()
				MARK_COLOR(bold_)
				MARK_COLOR(faint_)
				MARK_COLOR(bg_)
				default:    // GCOV_EXCL_LINE
					break;  // GCOV_EXCL_LINE
			}
		}
		return clr;
	}
}  // namespace cov
