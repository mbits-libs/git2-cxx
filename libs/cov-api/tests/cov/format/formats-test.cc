// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/chrono.h>
#include <gtest/gtest.h>
#include <cov/format.hh>
#include "../path-utils.hh"
#include "../print-view.hh"

namespace cov::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	namespace ph = placeholder;

	static sys_seconds now() {
		using clock = std::chrono::system_clock;
		static const auto now_ = std::chrono::floor<seconds>(clock::now());
		return now_;
	}

	struct format_test {
		std::string_view name;
		std::string_view tmplt{};
		std::string_view expected{};
		std::string_view no_facade{};
		struct {
			std::string_view report{};
			std::optional<io::v1::coverage_stats> stats{};
			std::string head{"feat/task-1"s};
			sys_seconds commit{};
			sys_seconds add{};
			std::optional<ph::rating> marks{};
			bool use_color{false};
			bool has_parent{true};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                format_test const& param) {
			return print_view(out << param.name << ", ", param.tmplt);
		}
	};

	class format : public TestWithParam<format_test> {
	protected:
		ph::environment environment(std::string const& head, bool use_color) {
			return {
			    .now = now(),
			    .hash_length = 9,
			    .names =
			        {
			            .HEAD = head,
			            .tags =
			                {{
			                     "v1.0.0"s,
			                     "221133445566778899aabbccddeeff0012345678"s,
			                 },
			                 {
			                     "v1.0.1"s,
			                     "112233445566778899aabbccddeeff0012345678"s,
			                 }},
			            .heads =
			                {{
			                     "main"s,
			                     "221144335566778899aabbccddeeff0012345678"s,
			                 },
			                 {
			                     "feat/task-1"s,
			                     "112233445566778899aabbccddeeff0012345678"s,
			                 }},
			            .HEAD_ref = "112233445566778899aabbccddeeff0012345678"s,
			        },
			    .colorize = use_color ? formatter::shell_colorize : nullptr,
			    .decorate = true,
			};
		}

		ref_ptr<cov::report> make_report(
		    git::oid_view id,
		    bool has_parent,
		    std::string const& message,
		    sys_seconds commit,
		    sys_seconds add,
		    std::optional<io::v1::coverage_stats> const& stats) const {
			git::oid parent_id{};
			if (has_parent)
				parent_id = git::oid::from(
				    "8765432100ffeeddccbbaa998877665544332211"sv);
			auto const commit_id =
			    git::oid::from("36109a1c35e0d5cf3e5e68d896c8b1b4be565525"sv);
			auto const files_id =
			    git::oid::from("7698a173c0f8b9c38bd853ba767c71df40b9f669"sv);

			io::v1::coverage_stats const default_stats{
			    1250, {300, 299}, {0, 0}, {0, 0}};
			return report::create(
			    id, parent_id, files_id, commit_id, "develop"sv,
			    {"Johnny Appleseed"sv, "johnny@appleseed.com"sv},
			    {"Johnny Committer"sv, "committer@appleseed.com"sv}, message,
			    commit, add, stats.value_or(default_stats), {});
		}
	};

	TEST_P(format, print) {
		auto const& [_, tmplt, expected, no_facade, tweaks] = GetParam();
		auto const& [report_id, stats, head, commit, add, marks, use_color,
		             has_parent] = tweaks;
		auto fmt = formatter::from(tmplt);

		ph::environment env = environment(head, use_color);
		if (marks) env.marks = *marks;
		auto const id = git::oid::from(
		    report_id.empty() ? "112233445566778899aabbccddeeff0012345678"sv
		                      : report_id);
		auto report = make_report(
		    id, has_parent,
		    "Subject, isn't it?\n\nLorem ipsum dolor sit amet, consectetur "
		    "adipiscing elit. Praesent facilisis\nfeugiat nibh in sodales. "
		    "Nullam non velit lectus. Morbi finibus risus vel\nrutrum "
		    "faucibus. Fusce commodo ultrices blandit. Nulla congue accumsan "
		    "risus,\nut tempus lorem facilisis quis. Nunc ultricies metus sed "
		    "lacinia lacinia.\nAenean eu euismod purus, sed pulvinar libero. "
		    "Sed a nibh sed tortor venenatis\nfaucibus ac sit amet massa. Sed "
		    "in fringilla ante. Integer tristique vulputate\nnulla nec "
		    "tristique. Ut finibus leo ut lacinia molestie. Mauris blandit "
		    "tortor\naugue, nec fermentum augue varius in. Nam ut ultrices "
		    "enim. Nulla facilisi.\nPhasellus auctor et arcu vel molestie. "
		    "Proin accumsan rutrum risus, vel\nsollicitudin sapien lobortis "
		    "eget.\n\nUt at odio id nisi malesuada varius ut ac mauris. Orci "
		    "varius natoque penatibus\net magnis dis parturient montes, "
		    "nascetur ridiculus mus. Duis pretium pretium\nfringilla. Nulla "
		    "vitae urna lacinia, viverra ex eu, sollicitudin "
		    "lorem.\nVestibulum ut elit consectetur, placerat turpis "
		    "vulputate, malesuada dui. Nulla\nsagittis nisi ut luctus cursus. "
		    "Quisque sodales sapien quis tempor efficitur.\nPraesent sit amet "
		    "mi ac erat dictum ultrices. Ut arcu nibh, blandit vitae "
		    "nisi\nsed, sollicitudin bibendum ligula.\n"s,
		    commit, add, stats);
		ASSERT_TRUE(report);

		auto facade = ph::object_facade::present_report(report, nullptr);
		auto actual = fmt.format(facade.get(), env);
		ASSERT_EQ(expected, actual);
	}

	TEST_P(format, print_no_facade) {
		auto const& [_, tmplt, expected, no_facade, tweaks] = GetParam();
		auto fmt = formatter::from(tmplt);

		ph::environment env = environment(tweaks.head, tweaks.use_color);
		if (tweaks.marks) env.marks = *tweaks.marks;

		auto actual = fmt.format(nullptr, env);
		ASSERT_EQ(no_facade, actual);
	}

	static format_test const tests[] = {
	    {"empty"sv},
	    {
	        .name = "HEAD"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445 (HEAD -> feat/task-1, tag: v1.0.1) 299/300 "
	            "100% (pass) - from [36109a1c3] Subject, isn't it? <Johnny "
	            "Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv},
	    },
	    {
	        .name = "HEAD (magic colors)"sv,
	        .tmplt = "%hR%md %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445\x1B[33m (\x1B[m\x1B[1;36mHEAD -> "
	            "\x1B[m\x1B[1;32mfeat/task-1\x1B[m\x1B[33m, "
	            "\x1B[m\x1B[1;33mtag: "
	            "v1.0.1\x1B[m\x1B[33m)\x1B[m 299/300 100% (pass) - from "
	            "[36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv,
	                   .use_color = true},
	    },
	    {
	        .name = "detached HEAD"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445 (HEAD, tag: v1.0.1, feat/task-1) 299/300 100% "
	            "(pass) - "
	            "from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv,
	                   .head = ""s},
	    },
	    {
	        .name = "detached HEAD (magic colors)"sv,
	        .tmplt = "%hR%md %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445\x1B[33m (\x1B[m\x1B[1;36mHEAD\x1B[m\x1B[33m, "
	            "\x1B[m\x1B[1;33mtag: v1.0.1\x1B[m\x1B[33m, "
	            "\x1B[m\x1B[1;32mfeat/task-1\x1B[m\x1B[33m)\x1B[m 299/300 100% "
	            "(pass) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv,
	                   .head = ""s,
	                   .use_color = true},
	    },
	    {
	        .name = "detached HEAD (magic colors unwrapped)"sv,
	        .tmplt = "%hR [%mD] %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445 [\x1B[1;36mHEAD\x1B[m\x1B[33m, \x1B[m\x1B[1;33mtag: "
	            "v1.0.1\x1B[m\x1B[33m, \x1B[m\x1B[1;32mfeat/task-1\x1B[m] "
	            "299/300 "
	            "100% (pass) - from [36109a1c3] Subject, isn't it? <Johnny "
	            "Appleseed>"sv,
	        .no_facade = " [] /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv,
	                   .head = ""s,
	                   .use_color = true},
	    },
	    {
	        .name = "main"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "221144335 (main) 226/300  75% (incomplete) - from [36109a1c3] "
	            "Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "221144335566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 226},
	                                                   {0, 0},
	                                                   {0, 0}}},
	    },
	    {
	        .name = "v1.0.0"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected = "221133445 (tag: v1.0.0) 270/300  90% (pass) - from "
	                    "[36109a1c3] "
	                    "Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "221133445566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 270},
	                                                   {0, 0},
	                                                   {0, 0}}},
	    },
	    {
	        .name = "no refs"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected = "442211335 100/300  33% (fail) - from [36109a1c3] "
	                    "Subject, isn't "
	                    "it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "442211335566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 100},
	                                                   {0, 0},
	                                                   {0, 0}}},
	    },
	    {
	        .name = "nothing to judge"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected = "442211335 0/0   0% (fail) - from [36109a1c3] Subject, "
	                    "isn't it? "
	                    "<Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks =
	            {.report = "442211335566778899aabbccddeeff0012345678"sv,
	             .stats = io::v1::coverage_stats{1250, {0, 0}, {0, 0}, {0, 0}}},
	    },
	    {
	        .name = "broken rating (bad passing)"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected = "442211335 300/300 100% (fail) - from [36109a1c3] "
	                    "Subject, isn't "
	                    "it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "442211335566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 300},
	                                                   {0, 0},
	                                                   {0, 0}},
	                   .marks{ph::rating{
	                       .lines{.incomplete{75, 100}, .passing{90, 0}},
	                       .functions{.incomplete{75, 100}, .passing{90, 0}},
	                       .branches{.incomplete{75, 100}, .passing{90, 0}}}}},
	    },
	    {
	        .name = "broken rating (bad incomplete)"sv,
	        .tmplt = "%hR%d %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected = "442211335 300/300 100% (fail) - from [36109a1c3] "
	                    "Subject, isn't "
	                    "it? <Johnny Appleseed>"sv,
	        .no_facade = " /  () - from []  <>"sv,
	        .tweaks = {.report = "442211335566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 300},
	                                                   {0, 0},
	                                                   {0, 0}},
	                   .marks{ph::rating{
	                       .lines{.incomplete{75, 100}, .passing{90, 0}},
	                       .functions{.incomplete{75, 100}, .passing{90, 0}},
	                       .branches{.incomplete{75, 100}, .passing{90, 0}}}}},
	    },
	    {
	        .name = "HEAD (unwrapped)"sv,
	        .tmplt = "%hR [%D] %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445 [HEAD -> feat/task-1, tag: v1.0.1] 299/300 100% "
	            "(pass) "
	            "- from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " [] /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv},
	    },
	    {
	        .name = "detached HEAD (unwrapped)"sv,
	        .tmplt = "%hR [%D] %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "112233445 [HEAD, tag: v1.0.1, feat/task-1] 299/300 100% "
	            "(pass) - "
	            "from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " [] /  () - from []  <>"sv,
	        .tweaks = {.report = "112233445566778899aabbccddeeff0012345678"sv,
	                   .head = ""s},
	    },
	    {
	        .name = "main (unwrapped)"sv,
	        .tmplt = "%hR [%D] %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "221144335 [main] 226/300  75% (incomplete) - from [36109a1c3] "
	            "Subject, isn't it? <Johnny Appleseed>"sv,
	        .no_facade = " [] /  () - from []  <>"sv,
	        .tweaks = {.report = "221144335566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 226},
	                                                   {0, 0},
	                                                   {0, 0}}},
	    },
	    {
	        .name = "ratios"sv,
	        .tmplt = "%pVL/%pTL %pPL (%prL)%n"
	                 "%pVF/%pTF %pPF (%prF)%n"
	                 "%pVB/%pTB %pPB (%prB)%n"sv,
	        .expected = "226/300  75% (incomplete)\n"
	                    "22/25  88% (incomplete)\n"
	                    "62672/125345  50% (fail)\n"sv,
	        .no_facade = "/  ()\n/  ()\n/  ()\n"sv,
	        .tweaks = {.stats = io::v1::coverage_stats{1250,
	                                                   {300, 226},
	                                                   {25, 22},
	                                                   {125'345, 62'672}}},
	    },
	    {
	        .name = "no refs (unwrapped)"sv,
	        .tmplt = "%hR [%D] %pVL/%pTL %pPL (%prL) - from [%hG] %s <%an>"sv,
	        .expected =
	            "442211335 [] 100/300  33% (fail) - from [36109a1c3] Subject, "
	            "isn't "
	            "it? <Johnny Appleseed>"sv,
	        .no_facade = " [] /  () - from []  <>"sv,
	        .tweaks = {.report = "442211335566778899aabbccddeeff0012345678"sv,
	                   .stats = io::v1::coverage_stats{1250,
	                                                   {300, 100},
	                                                   {0, 0},
	                                                   {0, 0}}},
	    },
	    {
	        .name = "file list (long and short)"sv,
	        .tmplt = "%HF %hF"sv,
	        .expected = "7698a173c0f8b9c38bd853ba767c71df40b9f669 7698a173c"sv,
	        .no_facade = " "sv,
	    },
	    {
	        .name = "export name"sv,
	        .tmplt = "0001-%hR-%f.xml"sv,
	        .expected = "0001-112233445-Subject-isn-t-it.xml"sv,
	        .no_facade = "0001--.xml"sv,
	    },
	    {
	        .name = "nary -- good"sv,
	        .tmplt = "%h1 %h2 %h3 %h4",
	        .expected = "112233445 7698a173c 876543210 36109a1c3"sv,
	        .no_facade = "   ",
	    },
	    {
	        .name = "nary -- no parent"sv,
	        .tmplt = "%h1 %h2 %h3 %h4",
	        .expected = "112233445 7698a173c  36109a1c3"sv,
	        .no_facade = "   ",
	        .tweaks = {.has_parent = false},
	    },
	    {
	        .name = "full body"sv,
	        .tmplt =
	            "%HR%d %pVL/%pTL %pPL (%prL) - from [%HG] %s <%an %al %ae>%n%B"sv,
	        .expected =
	            "112233445566778899aabbccddeeff0012345678 (HEAD -> "
	            "feat/task-1, "
	            "tag: v1.0.1) 299/300 100% (pass) - from "
	            "[36109a1c35e0d5cf3e5e68d896c8b1b4be565525] Subject, isn't it? "
	            "<Johnny Appleseed johnny johnny@appleseed.com>\nSubject, "
	            "isn't "
	            "it?\n\nLorem ipsum dolor sit amet, consectetur adipiscing "
	            "elit. "
	            "Praesent facilisis\nfeugiat nibh in sodales. Nullam non velit "
	            "lectus. Morbi finibus risus vel\nrutrum faucibus. Fusce "
	            "commodo "
	            "ultrices blandit. Nulla congue accumsan risus,\nut tempus "
	            "lorem "
	            "facilisis quis. Nunc ultricies metus sed lacinia "
	            "lacinia.\nAenean "
	            "eu euismod purus, sed pulvinar libero. Sed a nibh sed tortor "
	            "venenatis\nfaucibus ac sit amet massa. Sed in fringilla ante. "
	            "Integer tristique vulputate\nnulla nec tristique. Ut finibus "
	            "leo "
	            "ut lacinia molestie. Mauris blandit tortor\naugue, nec "
	            "fermentum "
	            "augue varius in. Nam ut ultrices enim. Nulla "
	            "facilisi.\nPhasellus "
	            "auctor et arcu vel molestie. Proin accumsan rutrum risus, "
	            "vel\nsollicitudin sapien lobortis eget.\n\nUt at odio id nisi "
	            "malesuada varius ut ac mauris. Orci varius natoque "
	            "penatibus\net "
	            "magnis dis parturient montes, nascetur ridiculus mus. Duis "
	            "pretium pretium\nfringilla. Nulla vitae urna lacinia, viverra "
	            "ex "
	            "eu, sollicitudin lorem.\nVestibulum ut elit consectetur, "
	            "placerat "
	            "turpis vulputate, malesuada dui. Nulla\nsagittis nisi ut "
	            "luctus "
	            "cursus. Quisque sodales sapien quis tempor "
	            "efficitur.\nPraesent "
	            "sit amet mi ac erat dictum ultrices. Ut arcu nibh, blandit "
	            "vitae "
	            "nisi\nsed, sollicitudin bibendum ligula."sv,
	        .no_facade = " /  () - from []  <  >\n"sv,
	    },
	    {
	        .name = "full printout"sv,
	        .tmplt = "report %HR (%rs)%n"
	                 "parent: %HP [%hP]%n"
	                 "stats: %pVL/%pTL (%pL) %pPL (%prL)%n"
	                 "commit: %hG%n"
	                 "  branch: %rD%n"
	                 "  author: %an <%ae> %at (%as)%n"
	                 "  committer: %cn <%ce> %ct (%cs)%n"
	                 "  subject: %s%n"
	                 "%n"
	                 "%w(60, 6)%b"sv,
	        .expected =
	            "report 112233445566778899aabbccddeeff0012345678 "
	            "(2000-02-29)\nparent: "
	            "8765432100ffeeddccbbaa998877665544332211 [876543210]\nstats: "
	            "299/300 (1250) 100% (pass)\ncommit: 36109a1c3\n  branch: "
	            "develop\n  author: Johnny Appleseed <johnny@appleseed.com> "
	            "951827696 (2000-02-29)\n  committer: Johnny Committer "
	            "<committer@appleseed.com> 951827696 (2000-02-29)\n  subject: "
	            "Subject, isn't it?\n\n      Lorem ipsum dolor sit amet, "
	            "consectetur adipiscing\n      elit. Praesent facilisis "
	            "feugiat "
	            "nibh in sodales.\n      Nullam non velit lectus. Morbi "
	            "finibus "
	            "risus vel\n      rutrum faucibus. Fusce commodo ultrices "
	            "blandit. "
	            "Nulla\n      congue accumsan risus, ut tempus lorem facilisis "
	            "quis.\n      Nunc ultricies metus sed lacinia lacinia. Aenean "
	            "eu\n      euismod purus, sed pulvinar libero. Sed a nibh "
	            "sed\n    "
	            "  tortor venenatis faucibus ac sit amet massa. Sed in\n      "
	            "fringilla ante. Integer tristique vulputate nulla nec\n      "
	            "tristique. Ut finibus leo ut lacinia molestie. Mauris\n      "
	            "blandit tortor augue, nec fermentum augue varius in.\n      "
	            "Nam "
	            "ut ultrices enim. Nulla facilisi. Phasellus auctor\n      et "
	            "arcu "
	            "vel molestie. Proin accumsan rutrum risus, vel\n      "
	            "sollicitudin sapien lobortis eget.\n\n      Ut at odio id "
	            "nisi "
	            "malesuada varius ut ac mauris. Orci\n      varius natoque "
	            "penatibus et magnis dis parturient\n      montes, nascetur "
	            "ridiculus mus. Duis pretium pretium\n      fringilla. Nulla "
	            "vitae "
	            "urna lacinia, viverra ex eu,\n      sollicitudin lorem. "
	            "Vestibulum ut elit consectetur,\n      placerat turpis "
	            "vulputate, "
	            "malesuada dui. Nulla\n      sagittis nisi ut luctus cursus. "
	            "Quisque sodales sapien\n      quis tempor efficitur. Praesent "
	            "sit "
	            "amet mi ac erat\n      dictum ultrices. Ut arcu nibh, blandit "
	            "vitae nisi sed,\n      sollicitudin bibendum ligula."
	            ""sv,
	        .no_facade = "report  ()\n"
	                     "parent:  []\n"
	                     "stats: / ()  ()\n"
	                     "commit: \n"
	                     "  branch: \n"
	                     "  author:  <>  ()\n"
	                     "  committer:  <>  ()\n"
	                     "  subject: \n"
	                     "\n"sv,
	        .tweaks = {.commit = sys_seconds{951827696s},
	                   .add = sys_seconds{951827706s}},
	    },
	    {
	        .name = "mid printout"sv,
	        .tmplt = "%hR %pPL (%prL) - [%rD] %an <%ae>%n%n%w(54,5,8)%B"sv,
	        .expected =
	            "112233445 100% (pass) - [develop] Johnny Appleseed "
	            "<johnny@appleseed.com>\n\n     Subject, isn't it?\n\n        "
	            "Lorem ipsum dolor sit amet, consectetur\n        adipiscing "
	            "elit. "
	            "Praesent facilisis feugiat\n        nibh in sodales. Nullam "
	            "non "
	            "velit lectus.\n        Morbi finibus risus vel rutrum "
	            "faucibus. "
	            "Fusce\n        commodo ultrices blandit. Nulla congue\n       "
	            " "
	            "accumsan risus, ut tempus lorem facilisis\n        quis. Nunc "
	            "ultricies metus sed lacinia\n        lacinia. Aenean eu "
	            "euismod "
	            "purus, sed pulvinar\n        libero. Sed a nibh sed tortor "
	            "venenatis\n        faucibus ac sit amet massa. Sed in "
	            "fringilla\n "
	            "       ante. Integer tristique vulputate nulla nec\n        "
	            "tristique. Ut finibus leo ut lacinia molestie.\n        "
	            "Mauris "
	            "blandit tortor augue, nec fermentum\n        augue varius in. "
	            "Nam "
	            "ut ultrices enim. Nulla\n        facilisi. Phasellus auctor "
	            "et "
	            "arcu vel\n        molestie. Proin accumsan rutrum risus, "
	            "vel\n    "
	            "    sollicitudin sapien lobortis eget.\n\n        Ut at odio "
	            "id "
	            "nisi malesuada varius ut ac\n        mauris. Orci varius "
	            "natoque "
	            "penatibus et\n        magnis dis parturient montes, "
	            "nascetur\n    "
	            "    ridiculus mus. Duis pretium pretium fringilla.\n        "
	            "Nulla "
	            "vitae urna lacinia, viverra ex eu,\n        sollicitudin "
	            "lorem. "
	            "Vestibulum ut elit\n        consectetur, placerat turpis "
	            "vulputate,\n        malesuada dui. Nulla sagittis nisi ut "
	            "luctus\n        cursus. Quisque sodales sapien quis tempor\n  "
	            "    "
	            "  efficitur. Praesent sit amet mi ac erat dictum\n        "
	            "ultrices. Ut arcu nibh, blandit vitae nisi\n        sed, "
	            "sollicitudin bibendum ligula."
	            ""sv,
	        .no_facade = "  () - []  <>\n\n"sv,
	        .tweaks = {.commit = sys_seconds{951827696s},
	                   .add = sys_seconds{951827706s}},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, format, ::testing::ValuesIn(tests));

	static constexpr auto feb29 =
	    date::sys_days{date::year{2000} / date::feb / 29} + 12h + 34min + 56s;

	TEST_F(format, long_line) {
		auto fmt = formatter::from("%w(30)%b"sv);

		ph::environment env = environment({}, false);
		auto const id =
		    git::oid::from("112233445566778899aabbccddeeff0012345678"sv);
		auto report = make_report(id, true,
		                          R"(Subject line

This-line-is-too-long-to-be-properly-wrapped. However, this line is perfectly wrappable)"s,
		                          feb29, feb29, std::nullopt);
		ASSERT_TRUE(report);
		static constexpr auto expected =
		    R"(      This-line-is-too-long-to-be-properly-wrapped.
      However, this line is
      perfectly wrappable)"sv;

		auto facade = ph::object_facade::present_report(report, nullptr);
		auto actual = fmt.format(facade.get(), env);
		ASSERT_EQ(expected, actual);
	}

	TEST_F(format, starts_excatly_at_limit) {
		auto fmt = formatter::from("%w(30, 5)%b"sv);

		ph::environment env = environment({}, false);
		auto const id =
		    git::oid::from("112233445566778899aabbccddeeff0012345678"sv);
		auto report = make_report(id, true,
		                          R"(Subject line

1234567890123456789012345 1234 56789 1234567890 987
5643 21.)"s,
		                          feb29, feb29, std::nullopt);
		ASSERT_TRUE(report);
		static constexpr auto expected =
		    R"(     1234567890123456789012345
     1234 56789 1234567890 987
     5643 21.)"sv;

		auto facade = ph::object_facade::present_report(report, nullptr);
		auto actual = fmt.format(facade.get(), env);
		ASSERT_EQ(expected, actual);
	}
}  // namespace cov::testing
