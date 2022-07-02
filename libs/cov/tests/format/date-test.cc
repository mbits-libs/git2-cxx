// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include "../path-utils.hh"
#include "../print-view.hh"

namespace cov::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	namespace ph = placeholder;

	static git_time_t now() {
		using clock = std::chrono::system_clock;
		static const git_time_t now_ = clock::to_time_t(clock::now());
		return now_;
	}

	struct date_test {
		std::string_view name;
		std::string_view tmplt{};
		std::string_view expected{};
		struct {
			git_time_t commit{};
			git_time_t add{};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                date_test const& param) {
			return print_view(out << param.name << ", ", param.tmplt);
		}
	};

	class format_date : public TestWithParam<date_test> {};

	TEST_P(format_date, print) {
		auto const& [_, tmplt, expected, tweaks] = GetParam();
		auto const& [commit, add] = tweaks;
		auto fmt = formatter::from(tmplt);

		ph::context ctx = {
		    .now = now(),
		    .hash_length = 9,
		    .names = {},
		};

		git_oid id{}, parent_id{}, commit_id{}, zero{};
		git_oid_fromstr(&id, "112233445566778899aabbccddeeff0012345678");
		git_oid_fromstr(&parent_id, "8765432100ffeeddccbbaa998877665544332211");
		git_oid_fromstr(&commit_id, "36109a1c35e0d5cf3e5e68d896c8b1b4be565525");

		auto report =
		    report_create(parent_id, zero, commit_id, "develop"s,
		                  "Johnny Appleseed"s, "johnny@appleseed.com"s,
		                  "Johnny Committer"s, "committer@appleseed.com"s,
		                  "Subject, isn't it?\n\nBody para 1\n\nBody para 2\n"s,
		                  commit, add, {1250, 300, 299});
		ASSERT_TRUE(report);

		auto view = ph::report_view::from(*report, &id);
		auto actual = fmt.format(view, ctx);
		ASSERT_EQ(expected, actual);
	}  // namespace cov::testing

	static date_test const tests[] = {
	    {
	        "in the future"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added in the future"sv,
	        {.add = now() + 1},
	    },
	    {
	        "1/2 min ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 31 seconds ago"sv,
	        {.add = now() - 31},
	    },
	    {
	        "5 min ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 5 minutes ago"sv,
	        {.add = now() - 300},
	    },
	    {
	        "hours ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 25 hours ago"sv,
	        {.add = now() - 25 * 3600},
	    },
	    {
	        "days ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 4 days ago"sv,
	        {.add = now() - 4 * 24 * 3600},
	    },
	    {
	        "weeks ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 2 weeks ago"sv,
	        {.add = now() - 14 * 24 * 3600},
	    },
	    {
	        "many weeks ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 9 weeks ago"sv,
	        {.add = now() - 65 * 24 * 3600},
	    },
	    {
	        "months ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 2 months ago"sv,
	        {.add = now() - 70 * 24 * 3600},
	    },
	    {
	        "year ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added one year ago"sv,
	        {.add = now() - 365 * 24 * 3600},
	    },
	    {
	        "2 years, 3 months ago"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 2 years, 3 months ago"sv,
	        {.add = now() - (2 * 365 + 80) * 24 * 3600},
	    },
	    {
	        "5+ years"sv,
	        "%hr (from %hc) %pP (%pr) added %rr"sv,
	        "112233445 (from 36109a1c3) 100% (pass) added 6 years ago"sv,
	        {.add = now() - (6 * 365 + 80) * 24 * 3600},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, format_date, ::testing::ValuesIn(tests));
}  // namespace cov::testing
