// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include "../path-utils.hh"
#include "../print-view.hh"

namespace cov::testing {
	using namespace ::std::literals;
	using namespace ::git::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	namespace ph = placeholder;

	static sys_seconds now() {
		using clock = std::chrono::system_clock;
		static const auto now_ = std::chrono::floor<seconds>(clock::now());
		return now_;
	}

	struct date_test {
		std::string_view name;
		std::string_view tmplt{};
		std::string_view expected{};
		struct {
			sys_seconds commit{};
			sys_seconds add{};
			std::string_view time_zone{};
			std::string_view locale{};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                date_test const& param) {
			return print_view(out << param.name << ", ", param.tmplt);
		}
	};

	class format_date : public TestWithParam<date_test> {};

	TEST_P(format_date, print) {
		auto const& [_, tmplt, expected, tweaks] = GetParam();
		auto const& [commit, add, time_zone, locale] = tweaks;
		auto fmt = formatter::from(tmplt);

		ph::environment env = {
		    .now = now(),
		    .hash_length = 9,
		    .names = {},
		    .time_zone = time_zone,
		    .locale = locale,
		};

		git::oid zero{};
		auto const id = "112233445566778899aabbccddeeff0012345678"_oid;
		auto const parent_id = "8765432100ffeeddccbbaa998877665544332211"_oid;
		auto const commit_id = "36109a1c35e0d5cf3e5e68d896c8b1b4be565525"_oid;

		auto report = report::create(
		    id, parent_id, zero, commit_id, "develop"sv,
		    {"Johnny Appleseed"sv, "johnny@appleseed.com"sv},
		    {"Johnny Committer"sv, "committer@appleseed.com"sv},
		    "Subject, isn't it?\n\nBody para 1\n\nBody para 2\n"sv, commit, add,
		    {1250, {300, 299}, {0, 0}, {0, 0}}, {});
		ASSERT_TRUE(report);

		fmt::print("locale: {}\n", env.locale);
		auto facade = ph::object_facade::present_report(report, nullptr);
		auto actual = fmt.format(facade.get(), env);
		ASSERT_EQ(expected, actual);
	}  // namespace cov::testing

	using days = std::chrono::duration<
	    std::chrono::hours::rep,
	    std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>>;

	constexpr days operator""_d(unsigned long long val) { return days{val}; }

	using date::sys_days;
	using date::operator""_y;
	using date::feb;

	static constexpr auto feb29 =
	    sys_days{2000_y / feb / 29} + 12h + 34min + 56s;

	// If you'll get locale::facet::_S_create_c_locale name not valid exceptions
	// on some of those tests, on Ubuntu:
	// ```
	// sudo locale-gen "pl_PL.UTF-8" "en_US.UTF-8" "en_GB.UTF-8"
	// ```

	static date_test const tests[] = {
#ifndef CUTDOWN_OS
	    {
	        "Date in Poland/Polish"sv,
	        "%rd"sv,
#ifdef _WIN32
	        "29.02.2000 13:34:56"sv,
#else
	        "wto, 29 lut 2000, 13:34:56"sv,
#endif
	        {.add = feb29,
	         .time_zone = "Europe/Warsaw"sv,
	         .locale = "pl_PL.UTF-8"sv},
	    },
	    {
	        "Date in Poland/UK English"sv,
	        "%rd"sv,
#ifdef _WIN32
	        "29/02/2000 13:34:56"sv,
#else
	        "Tue 29 Feb 2000 13:34:56 CET"sv,
#endif
	        {.add = feb29,
	         .time_zone = "Europe/Warsaw"sv,
	         .locale = "en_GB.UTF-8"sv},
	    },
#endif  // CUTDOWN_OS
	    {
	        "Date in Poland/US English"sv,
	        "%rd"sv,
#ifdef _WIN32
	        "2/29/2000 1:34:56 PM"sv,
#else
	        "Tue 29 Feb 2000 01:34:56 PM CET"sv,
#endif
	        {.add = feb29,
	         .time_zone = "Europe/Warsaw"sv,
	         .locale = "en_US.UTF-8"sv},
	    },
#ifndef CUTDOWN_OS
	    {
	        "Date in Labrador/Polish"sv,
	        "%rd"sv,
#ifdef _WIN32
	        "29.02.2000 09:04:56"sv,
#else
	        "wto, 29 lut 2000, 09:04:56"sv,
#endif
	        {.add = feb29,
	         .time_zone = "America/St_Johns"sv,
	         .locale = "pl_PL.UTF-8"sv},
	    },
	    {
	        "Date in Labrador/UK English"sv,
	        "%rd"sv,
#ifdef _WIN32
	        "29/02/2000 09:04:56"sv,
#else
	        "Tue 29 Feb 2000 09:04:56 NST"sv,
#endif
	        {.add = feb29,
	         .time_zone = "America/St_Johns"sv,
	         .locale = "en_GB.UTF-8"sv},
	    },
#endif  // CUTDOWN_OS
	    {
	        "Date in Labrador/US English"sv,
	        "%rd"sv,
#ifdef _WIN32
	        "2/29/2000 9:04:56 AM"sv,
#else
	        "Tue 29 Feb 2000 09:04:56 AM NST"sv,
#endif
	        {.add = feb29,
	         .time_zone = "America/St_Johns"sv,
	         .locale = "en_US.UTF-8"sv},
	    },
	    {
	        "Date in Poland/C"sv,
	        "%rd"sv,
	        "Tue Feb 29 13:34:56 2000"sv,
	        {.add = feb29, .time_zone = "Europe/Warsaw"sv, .locale = "C"sv},
	    },
	    {
	        "ISO in Labrador"sv,
	        "%rI"sv,
	        "2000-02-29T09:04:56-0430"sv,
	        {.add = feb29, .time_zone = "America/St_Johns"sv},
	    },
	    {
	        "ISO in Poland"sv,
	        "%rI"sv,
	        "2000-02-29T13:34:56+0100"sv,
	        {.add = feb29, .time_zone = "Europe/Warsaw"sv},
	    },
	    {
	        "ISO in Kathmandu"sv,
	        "%rI"sv,
	        "2000-02-29T18:19:56+0545"sv,
	        {.add = feb29, .time_zone = "Asia/Kathmandu"sv},
	    },
	    {
	        "ISO-like in Labrador"sv,
	        "%ri"sv,
	        "2000-02-29 09:04:56 -04:30"sv,
	        {.add = feb29, .time_zone = "America/St_Johns"sv},
	    },
	    {
	        "ISO-like in Poland"sv,
	        "%ri"sv,
	        "2000-02-29 13:34:56 +01:00"sv,
	        {.add = feb29, .time_zone = "Europe/Warsaw"sv},
	    },
	    {
	        "ISO-like in Kathmandu"sv,
	        "%ri"sv,
	        "2000-02-29 18:19:56 +05:45"sv,
	        {.add = feb29, .time_zone = "Asia/Kathmandu"sv},
	    },
	    {
	        "in the future"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added in the future"sv,
	        {.add = now() + 1s},
	    },
	    {
	        "1/2 min ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 31 seconds ago"sv,
	        {.add = now() - 31s},
	    },
	    {
	        "5 min ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 5 minutes ago"sv,
	        {.add = now() - 5min},
	    },
	    {
	        "hours ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 25 hours ago"sv,
	        {.add = now() - 25h},
	    },
	    {
	        "days ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 4 days ago"sv,
	        {.add = now() - 4_d},
	    },
	    {
	        "weeks ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 2 weeks ago"sv,
	        {.add = now() - 14_d},
	    },
	    {
	        "many weeks ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 9 weeks ago"sv,
	        {.add = now() - 65_d},
	    },
	    {
	        "months ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 2 months ago"sv,
	        {.add = now() - 70_d},
	    },
	    {
	        "year ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added one year ago"sv,
	        {.add = now() - 365_d},
	    },
	    {
	        "2 years, 3 months ago"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 2 years, 3 months ago"sv,
	        {.add = now() - (2 * 365_d + 80_d)},
	    },
	    {
	        "5+ years"sv,
	        "%hR (from %hG) %pPL (%prL) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 6 years ago"sv,
	        {.add = now() - (6 * 365_d + 80_d)},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, format_date, ::testing::ValuesIn(tests));
}  // namespace cov::testing
