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
#include <cov/repository.hh>
#include <ctime>
#include <iostream>
#include "../path-utils.hh"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#define _isatty(FD) isatty(FD)
#define _fileno(OBJ) fileno(OBJ)
#endif

namespace cov {
	namespace placeholder {
		namespace {
			bool is_terminal(FILE* out) noexcept {
#ifndef _WIN32
				char const* term = getenv("TERM");
#endif
				return (_isatty(_fileno(out)) != 0)
#ifndef _WIN32
				       && term && strcmp(term, "dumb") != 0;
#endif
				;
			}

			refs names_from(cov::repository const& repo) {
				refs result{};
				char buffer[GIT_OID_HEXSZ];

				auto HEAD = repo.current_head();
				result.HEAD = std::move(HEAD.branch);

				if (HEAD.tip) {
					git_oid_fmt(buffer, &*HEAD.tip);
					result.HEAD_ref.assign(buffer, GIT_OID_HEXSZ);
				}

				struct name {
					std::map<std::string, std::string>* dst;
					std::string_view prefix;
				};

				name const names[] = {
				    {&result.heads, "refs/heads/"sv},
				    {&result.tags, "refs/tags/"sv},
				};

				for (auto const [dst, prefix] : names) {
					auto iter = repo.refs()->iterator(prefix);

					for (auto ref : *iter) {
						std::string name{};
						name.assign(ref->shorthand());
						while (ref->reference_type() ==
						       reference_type::symbolic)
							ref = ref->peel_target();

						if (ref->reference_type() != reference_type::direct)
							continue;

						git_oid_fmt(buffer, ref->direct_target());
						(*dst)[name].assign(buffer, GIT_OID_HEXSZ);
					}
				}

				return result;
			}

			std::string_view stripped(std::string_view value) {
				while (!value.empty() &&
				       std::isspace(static_cast<unsigned>(value.front())))
					value = value.substr(1);
				while (!value.empty() &&
				       std::isspace(static_cast<unsigned>(value.back())))
					value = value.substr(0, value.size() - 1);
				return value;
			}

			std::optional<unsigned> number_from(std::string_view value) {
				value = stripped(value);
				unsigned result{};
				if (value.empty()) return std::nullopt;
				auto ptr = value.data();
				auto end = ptr + value.size();
				auto [finish, errc] = std::from_chars(ptr, end, result);
				if (errc == std::errc{} && finish == end) return result;
				return std::nullopt;
			};

			std::optional<ratio> ratio_from(std::string_view value) {
				auto const pos = value.find('/');
				if (pos == std::string_view::npos) {
					value = stripped(value);
					if (!value.empty() && value.back() == '%') {
						auto const percent =
						    number_from(value.substr(0, value.size() - 1));
						if (percent) return ratio{*percent, 100u}.gcd();
					}

					return std::nullopt;
				}

				auto const numerator = number_from(value.substr(0, pos));
				auto const denominator = number_from(value.substr(pos + 1));

				if (!numerator || !denominator || !*denominator)
					return std::nullopt;

				return ratio{*numerator, *denominator}.gcd();
			};

			rating rating_from(cov::repository const& repo) {
				auto const value = repo.config().get_string("core.rating");
				if (value) {
					auto const view = std::string_view{*value};
					auto const pos = view.find(',');
					if (pos != std::string_view::npos) {
						auto const one = ratio_from(view.substr(0, pos));
						auto const other = ratio_from(view.substr(pos + 1));
						if (one && other) {
							auto const nums_gcd =
							    std::gcd(one->num, other->num);
							auto const dens_gcd =
							    std::gcd(one->den, other->den);
							auto const one_gcd =
							    ratio{one->num / nums_gcd, one->den / dens_gcd};
							auto const other_gcd = ratio{other->num / nums_gcd,
							                             other->den / dens_gcd};

							if ((one_gcd.num * other_gcd.den) >
							    (other_gcd.num * one_gcd.den)) {
								// one > other
								return {.incomplete = *other, .passing = *one};
							}
							// one <= other
							return {.incomplete = *one, .passing = *other};
						}
					}
				}

				// default: 75% and 90%
				return {.incomplete = {3, 4}, .passing = {9, 10}};
			};
		}  // namespace

		context context::from(cov::repository const& repo,
		                      color_feature clr,
		                      decorate_feature decorate) {
			if (clr == use_feature::automatic) {
				clr = is_terminal(stdout) ? use_feature::yes : use_feature::no;
			}

			if (decorate == use_feature::automatic) {
				decorate = is_terminal(stdout) ? use_feature::yes : use_feature::no;
			}

			using namespace std::chrono;
			std::string (*colorize)(color, void* app) =
			    clr == use_feature::yes ? formatter::shell_colorize : nullptr;
			return {.now = floor<seconds>(system_clock::now()),
			        .hash_length = 9,
			        .names = names_from(repo),
			        .marks = rating_from(repo),
			        .colorize = colorize,
			        .decorate = decorate == use_feature::yes};
		}
	}  // namespace placeholder

	namespace {
		struct tz_visitor {
			bool operator()(char) { return false; }

			bool operator()(std::string const&) { return false; }

			bool operator()(placeholder::person_info const& pair) {
				using placeholder::person;
				switch (pair.second) {
					case person::date:
					case person::date_iso_like:
					case person::date_iso_strict:
					case person::date_short:
						return true;
					default:
						return false;
				}
			}
			bool operator()(auto) { return false; }
			bool operator()(placeholder::width const&) { return false; }
		};

		std::string default_tr(long long count,
		                       std::string_view singular,
		                       std::string_view plural) {
			if (count == 1) return fmt::format(fmt::runtime(singular), count);
			return fmt::format(fmt::runtime(plural), count);
		}
	}  // namespace

	formatter::formatter(std::vector<placeholder::format>&& format)
	    : format_{std::move(format)} {
		needs_timezones_ = false;
		for (auto const& fmt : format_)
			needs_timezones_ |= std::visit(tz_visitor{}, fmt);
	}

	std::string formatter::shell_colorize(placeholder::color clr, void*) {
		static constexpr std::string_view colors[] = {
		    ""sv,            // color::normal
		    "\033[m"sv,      // color::reset
		    "\033[31m"sv,    // color::red
		    "\033[32m"sv,    // color::green
		    "\033[33m"sv,    // color::yellow
		    "\033[34m"sv,    // color::blue
		    "\033[35m"sv,    // color::magenta
		    "\033[36m"sv,    // color::cyan
		    "\033[1;37m"sv,  // color::bold_normal,
		    "\033[1;31m"sv,  // color::bold_red
		    "\033[1;32m"sv,  // color::bold_green
		    "\033[1;33m"sv,  // color::bold_yellow
		    "\033[1;34m"sv,  // color::bold_blue
		    "\033[1;35m"sv,  // color::bold_magenta
		    "\033[1;36m"sv,  // color::bold_cyan
		    "\033[2;37m"sv,  // color::faint_normal,
		    "\033[2;31m"sv,  // color::faint_red
		    "\033[2;32m"sv,  // color::faint_green
		    "\033[2;33m"sv,  // color::faint_yellow
		    "\033[2;34m"sv,  // color::faint_blue
		    "\033[2;35m"sv,  // color::faint_magenta
		    "\033[2;36m"sv,  // color::faint_cyan
		    "\033[41m"sv,    // color::bg_red
		    "\033[42m"sv,    // color::bg_green
		    "\033[43m"sv,    // color::bg_yellow
		    "\033[44m"sv,    // color::bg_blue
		    "\033[45m"sv,    // color::bg_magenta
		    "\033[46m"sv,    // color::bg_cyan
		};
		auto const index = static_cast<size_t>(clr);
		auto const color =
		    index >= std::size(colors) ? colors[0] : colors[index];
		return {color.data(), color.size()};
	}

	std::string formatter::no_translation(long long count,
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
				return default_tr(count, "one hour ago"sv, "{} hours ago"sv);
			case translatable::days_ago:
				return default_tr(count, "one day ago"sv, "{} days ago"sv);
			case translatable::weeks_ago:
				return default_tr(count, "one week ago"sv, "{} weeks ago"sv);
			case translatable::months_ago:
				return default_tr(count, "one month ago"sv, "{} months ago"sv);
			case translatable::years_months_ago:
				return default_tr(count, "one year,"sv, "{} years,"sv);
			case translatable::years_ago:
				return default_tr(count, "one year ago"sv, "{} years ago"sv);
			case translatable::mark_failing:
				return "fail";
			case translatable::mark_incomplete:
				return "incomplete";
			case translatable::mark_passing:
				return "pass";
		}
		// all enums are handled above
		return std::to_string(count);  // GCOV_EXCL_LINE
	}
}  // namespace cov
