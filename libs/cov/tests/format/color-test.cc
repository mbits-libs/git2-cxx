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

	struct color_test {
		std::string_view name;
		std::string_view tmplt{};
		struct {
			std::string_view nocolor;
			std::string_view shell;
		} expected{};
		struct {
			std::optional<io::v1::coverage_stats> stats{};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                color_test const& param) {
			return out << param.name;
		}
	};

	using namespace date;

	static constexpr auto feb29 =
	    sys_days{2000_y / feb / 29} + 12h + 34min + 56s;

	static constexpr auto jul5 = sys_days{2000_y / jul / 5} + 12h;

	class format_color : public TestWithParam<color_test> {
	protected:
		ref_ptr<cov::report> make_report(
		    git_oid const& id,
		    std::optional<io::v1::coverage_stats> const& stats) const {
			git_oid parent_id{}, commit_id{}, zero{};
			git_oid_fromstr(&parent_id,
			                "8765432100ffeeddccbbaa998877665544332211");
			git_oid_fromstr(&commit_id,
			                "36109a1c35e0d5cf3e5e68d896c8b1b4be565525");

			io::v1::coverage_stats const default_stats{1250, 300, 299};
			return report::create(
			    id, parent_id, zero, commit_id, "develop"sv,
			    {"Johnny Appleseed"sv, "johnny@appleseed.com"sv},
			    {"Johnny Committer"sv, "committer@appleseed.com"sv},
			    "Subject, isn't it?\n\nBody para 1\n\nBody para 2\n"sv, feb29,
			    feb29, stats.value_or(default_stats));
		}
	};

	TEST_P(format_color, nocolor) {
		auto const& [_, tmplt, expected, tweaks] = GetParam();
		auto fmt = formatter::from(tmplt);

		ph::context ctx = {
		    .now = jul5,
		    .hash_length = 9,
		    .names = {.HEAD = {},
		              .tags = {},
		              .heads =
		                  {
		                      {"main"s,
		                       "112233445566778899aabbccddeeff0012345678"s},
		                  },
		              .HEAD_ref = "112233445566778899aabbccddeeff0012345678"s},
		    .decorate = true,
		};

		git_oid id{};
		git_oid_fromstr(&id, "112233445566778899aabbccddeeff0012345678");
		auto report = make_report(id, tweaks.stats);
		ASSERT_TRUE(report);

		auto view = ph::report_view::from(*report);
		auto actual = fmt.format(view, ctx);
		std::fputs(actual.c_str(), stdout);
		std::fputc('\n', stdout);
		ASSERT_EQ(expected.nocolor, actual);
	}

	TEST_P(format_color, shell) {
		auto const& [_, tmplt, expected, tweaks] = GetParam();
		auto fmt = formatter::from(tmplt);

		ph::context ctx = {
		    .now = jul5,
		    .hash_length = 9,
		    .names = {.HEAD = {},
		              .tags = {},
		              .heads =
		                  {
		                      {"main"s,
		                       "112233445566778899aabbccddeeff0012345678"s},
		                  },
		              .HEAD_ref = "112233445566778899aabbccddeeff0012345678"s},
		    .colorize = formatter::shell_colorize,
		};

		git_oid id{};
		git_oid_fromstr(&id, "112233445566778899aabbccddeeff0012345678");
		auto report = make_report(id, tweaks.stats);
		ASSERT_TRUE(report);

		auto view = ph::report_view::from(*report);
		auto actual = fmt.format(view, ctx);
		std::fputs(actual.c_str(), stdout);
		std::fputc('\n', stdout);
		ASSERT_EQ(expected.shell, actual);
	}

	TEST(format_color, apply_mark) {
		using placeholder::color;
		placeholder::rating rating{.incomplete{75, 100}, .passing{9, 10}};
		struct mark_info {
			color clr, expected;
			io::v1::coverage_stats stats;
		};

		static constexpr mark_info marks[] = {
		    {
		        .clr = color::rating,
		        .expected = color::green,
		        .stats{.total{}, .relevant{100}, .covered{100}},
		    },
		    {
		        .clr = color::bg_rating,
		        .expected = color::bg_yellow,
		        .stats{.total{}, .relevant{100}, .covered{89}},
		    },
		    {
		        .clr = color::faint_rating,
		        .expected = color::faint_red,
		        .stats{.total{}, .relevant{100}, .covered{70}},
		    },
		    {
		        .clr = color::blue,
		        .expected = color::blue,
		        .stats{.total{}, .relevant{100}, .covered{70}},
		    }};

		for (auto const [clr, expected, stats] : marks) {
			auto const actual = formatter::apply_mark(clr, stats, rating);
			EXPECT_EQ(expected, actual);
		}
	}

	using days = std::chrono::duration<
	    std::chrono::hours::rep,
	    std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>>;

	static constexpr auto rating_tmplt =
	    "%C(red)%hr%Creset%C(yellow)%d%Creset %C(bg rating) %pP %Creset "
	    "%C(faint normal)%pC/%pR%Creset %C(bold rating)(%pr)%Creset - "
	    "%Cred[%hc@%rD]%Creset %s %Cblue<%an>%Creset %C(green)(committed %cr, "
	    "added %rr)%C(reset)"sv;

	static color_test const tests[] = {
	    {
	        "RGB"sv,
	        "%CredR%CgreenG%CblueB%Creset"sv,
	        {"RGB"sv, "\x1B[31mR\x1B[32mG\x1B[34mB\x1B[m"sv},
	    },
	    {
	        "Rating: pass"sv,
	        rating_tmplt,
	        {
	            "112233445 (HEAD, main)  100%  299/300 (pass) - "
	            "[36109a1c3@develop] Subject, isn't it? <Johnny Appleseed> "
	            "(committed 4 months ago, added 4 months ago)"sv,
	            "\x1B[31m112233445\x1B[m\x1B[33m\x1B[m \x1B[42m 100% \x1B[m "
	            "\x1B[2;37m299/300\x1B[m \x1B[1;32m(pass)\x1B[m - "
	            "\x1B[31m[36109a1c3@develop]\x1B[m Subject, isn't it? "
	            "\x1B[34m<Johnny Appleseed>\x1B[m \x1B[32m(committed 4 months "
	            "ago, added 4 months ago)\x1B[m"sv,
	        },
	    },
	    {
	        "Rating: incomplete"sv,
	        rating_tmplt,
	        {
	            "112233445 (HEAD, main)   79%  790/1000 (incomplete) - "
	            "[36109a1c3@develop] Subject, isn't it? <Johnny Appleseed> "
	            "(committed 4 months ago, added 4 months ago)"sv,
	            "\x1B[31m112233445\x1B[m\x1B[33m\x1B[m \x1B[43m  79% \x1B[m "
	            "\x1B[2;37m790/1000\x1B[m \x1B[1;33m(incomplete)\x1B[m - "
	            "\x1B[31m[36109a1c3@develop]\x1B[m Subject, isn't it? "
	            "\x1B[34m<Johnny Appleseed>\x1B[m \x1B[32m(committed 4 months "
	            "ago, added 4 months ago)\x1B[m"sv,
	        },
	        {.stats = io::v1::coverage_stats{1250, 1000, 790}},
	    },
	    {
	        "Rating: bad"sv,
	        rating_tmplt,
	        {
	            "112233445 (HEAD, main)   50%  300/600 (fail) - "
	            "[36109a1c3@develop] Subject, isn't it? <Johnny Appleseed> "
	            "(committed 4 months ago, added 4 months ago)"sv,
	            "\x1B[31m112233445\x1B[m\x1B[33m\x1B[m \x1B[41m  50% \x1B[m "
	            "\x1B[2;37m300/600\x1B[m \x1B[1;31m(fail)\x1B[m - "
	            "\x1B[31m[36109a1c3@develop]\x1B[m Subject, isn't it? "
	            "\x1B[34m<Johnny Appleseed>\x1B[m \x1B[32m(committed 4 months "
	            "ago, added 4 months ago)\x1B[m"sv,
	        },
	        {.stats = io::v1::coverage_stats{1250, 600, 300}},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, format_color, ::testing::ValuesIn(tests));
}  // namespace cov::testing
