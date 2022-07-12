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

	static sys_seconds now() {
		using clock = std::chrono::system_clock;
		static const auto now_ = std::chrono::floor<seconds>(clock::now());
		return now_;
	}

	struct format_test {
		std::string_view name;
		std::string_view tmplt{};
		std::string_view expected{};
		struct {
			std::string_view report{};
			std::optional<io::v1::coverage_stats> stats{};
			std::string head{"feat/task-1"s};
			sys_seconds commit{};
			sys_seconds add{};
			std::optional<ph::rating> marks{};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                format_test const& param) {
			return print_view(out << param.name << ", ", param.tmplt);
		}
	};

	class format : public TestWithParam<format_test> {
	protected:
		ph::context context(std::string const& head) {
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
			};
		}

		ref_ptr<cov::report> make_report(
		    std::string const& message,
		    sys_seconds commit,
		    sys_seconds add,
		    std::optional<io::v1::coverage_stats> const& stats) const {
			git_oid parent_id{}, commit_id{}, zero{};
			git_oid_fromstr(&parent_id,
			                "8765432100ffeeddccbbaa998877665544332211");
			git_oid_fromstr(&commit_id,
			                "36109a1c35e0d5cf3e5e68d896c8b1b4be565525");

			io::v1::coverage_stats const default_stats{1250, 300, 299};
			return report_create(parent_id, zero, commit_id, "develop"s,
			                     "Johnny Appleseed"s, "johnny@appleseed.com"s,
			                     "Johnny Committer"s,
			                     "committer@appleseed.com"s, message, commit,
			                     add, stats.value_or(default_stats));
		}
	};

	TEST_P(format, print) {
		auto const& [_, tmplt, expected, tweaks] = GetParam();
		auto const& [report_id, stats, head, commit, add, marks] = tweaks;
		auto fmt = formatter::from(tmplt);

		ph::context ctx = context(head);
		if (marks) ctx.marks = *marks;
		git_oid id{};
		git_oid_fromstr(&id, report_id.empty()
		                         ? "112233445566778899aabbccddeeff0012345678"
		                         : report_id.data());
		auto report = make_report(
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

		auto view = ph::report_view::from(*report, &id);
		auto actual = fmt.format(view, ctx);
		ASSERT_EQ(expected, actual);
	}  // namespace cov::testing

	static format_test const tests[] = {
	    {"empty"sv},
	    {
	        "HEAD"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "112233445 (HEAD -> feat/task-1, tag: v1.0.1, feat/task-1) 299/300 100% (pass) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "112233445566778899aabbccddeeff0012345678"sv},
	    },
	    {
	        "detached HEAD"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "112233445 (HEAD, tag: v1.0.1, feat/task-1) 299/300 100% (pass) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "112233445566778899aabbccddeeff0012345678"sv,
	         .head = ""s},
	    },
	    {
	        "main"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "221144335 (main) 226/300 75% (incomplete) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "221144335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 226}},
	    },
	    {
	        "v1.0.0"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "221133445 (tag: v1.0.0) 270/300 90% (pass) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "221133445566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 270}},
	    },
	    {
	        "no refs"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "442211335 100/300 33% (fail) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "442211335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 100}},
	    },
	    {
	        "nothing to judge"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "442211335 0/0  0% (fail) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "442211335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 0, 0}},
	    },
	    {
	        "broken rating (bad passing)"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "442211335 300/300 100% (fail) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "442211335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 300},
	         .marks{ph::rating{.incomplete{75, 100}, .passing{90, 0}}}},
	    },
	    {
	        "broken rating (bad incomplete)"sv,
	        "%hr%d %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "442211335 300/300 100% (fail) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "442211335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 300},
	         .marks{ph::rating{.incomplete{75, 0}, .passing{9, 10}}}},
	    },
	    {
	        "HEAD (unwrapped)"sv,
	        "%hr [%D] %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "112233445 [HEAD -> feat/task-1, tag: v1.0.1, feat/task-1] 299/300 100% (pass) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "112233445566778899aabbccddeeff0012345678"sv},
	    },
	    {
	        "detached HEAD (unwrapped)"sv,
	        "%hr [%D] %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "112233445 [HEAD, tag: v1.0.1, feat/task-1] 299/300 100% (pass) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "112233445566778899aabbccddeeff0012345678"sv,
	         .head = ""s},
	    },
	    {
	        "main (unwrapped)"sv,
	        "%hr [%D] %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "221144335 [main] 226/300 75% (incomplete) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "221144335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 226}},
	    },
	    {
	        "no refs (unwrapped)"sv,
	        "%hr [%D] %pC/%pR %pP (%pr) - from [%hc] %s <%an>"sv,
	        "442211335 [] 100/300 33% (fail) - from [36109a1c3] Subject, isn't it? <Johnny Appleseed>"sv,
	        {.report = "442211335566778899aabbccddeeff0012345678"sv,
	         .stats = io::v1::coverage_stats{1250, 300, 100}},
	    },
	    {
	        "export name"sv,
	        "0001-%hr-%f.xml"sv,
	        "0001-112233445-Subject-isn-t-it.xml"sv,
	    },
	    {
	        "full body"sv,
	        "%Hr%d %pC/%pR %pP (%pr) - from [%Hc] %s <%an %al %ae>%n%B"sv,
	        "112233445566778899aabbccddeeff0012345678 (HEAD -> feat/task-1, "
	        "tag: v1.0.1, feat/task-1) 299/300 100% (pass) - from "
	        "[36109a1c35e0d5cf3e5e68d896c8b1b4be565525] Subject, isn't it? "
	        "<Johnny Appleseed johnny johnny@appleseed.com>\nSubject, isn't "
	        "it?\n\nLorem ipsum dolor sit amet, consectetur adipiscing elit. "
	        "Praesent facilisis\nfeugiat nibh in sodales. Nullam non velit "
	        "lectus. Morbi finibus risus vel\nrutrum faucibus. Fusce commodo "
	        "ultrices blandit. Nulla congue accumsan risus,\nut tempus lorem "
	        "facilisis quis. Nunc ultricies metus sed lacinia lacinia.\nAenean "
	        "eu euismod purus, sed pulvinar libero. Sed a nibh sed tortor "
	        "venenatis\nfaucibus ac sit amet massa. Sed in fringilla ante. "
	        "Integer tristique vulputate\nnulla nec tristique. Ut finibus leo "
	        "ut lacinia molestie. Mauris blandit tortor\naugue, nec fermentum "
	        "augue varius in. Nam ut ultrices enim. Nulla facilisi.\nPhasellus "
	        "auctor et arcu vel molestie. Proin accumsan rutrum risus, "
	        "vel\nsollicitudin sapien lobortis eget.\n\nUt at odio id nisi "
	        "malesuada varius ut ac mauris. Orci varius natoque penatibus\net "
	        "magnis dis parturient montes, nascetur ridiculus mus. Duis "
	        "pretium pretium\nfringilla. Nulla vitae urna lacinia, viverra ex "
	        "eu, sollicitudin lorem.\nVestibulum ut elit consectetur, placerat "
	        "turpis vulputate, malesuada dui. Nulla\nsagittis nisi ut luctus "
	        "cursus. Quisque sodales sapien quis tempor efficitur.\nPraesent "
	        "sit amet mi ac erat dictum ultrices. Ut arcu nibh, blandit vitae "
	        "nisi\nsed, sollicitudin bibendum ligula."
	        ""sv,
	    },
	    {
	        "full printout"sv,
	        "report %Hr%n"
	        "parent: %rP [%rp]%n"
	        "stats: %pC/%pR (%pT) %pP (%pr)%n"
	        "commit: %hc%n"
	        "  branch: %rD%n"
	        "  author: %an <%ae> %at (%as)%n"
	        "  committer: %cn <%ce> %ct (%cs)%n"
	        "  subject: %s%n"
	        "%n"
	        "%w(60, 6)%b"sv,
	        "report 112233445566778899aabbccddeeff0012345678\nparent: "
	        "8765432100ffeeddccbbaa998877665544332211 [876543210]\nstats: "
	        "299/300 (1250) 100% (pass)\ncommit: 36109a1c3\n  branch: "
	        "develop\n  author: Johnny Appleseed <johnny@appleseed.com> "
	        "951827696 (2000-02-29)\n  committer: Johnny Committer "
	        "<committer@appleseed.com> 951827696 (2000-02-29)\n  subject: "
	        "Subject, isn't it?\n\n      Lorem ipsum dolor sit amet, "
	        "consectetur adipiscing\n      elit. Praesent facilisis feugiat "
	        "nibh in sodales.\n      Nullam non velit lectus. Morbi finibus "
	        "risus vel\n      rutrum faucibus. Fusce commodo ultrices blandit. "
	        "Nulla\n      congue accumsan risus, ut tempus lorem facilisis "
	        "quis.\n      Nunc ultricies metus sed lacinia lacinia. Aenean "
	        "eu\n      euismod purus, sed pulvinar libero. Sed a nibh sed\n    "
	        "  tortor venenatis faucibus ac sit amet massa. Sed in\n      "
	        "fringilla ante. Integer tristique vulputate nulla nec\n      "
	        "tristique. Ut finibus leo ut lacinia molestie. Mauris\n      "
	        "blandit tortor augue, nec fermentum augue varius in.\n      Nam "
	        "ut ultrices enim. Nulla facilisi. Phasellus auctor\n      et arcu "
	        "vel molestie. Proin accumsan rutrum risus, vel\n      "
	        "sollicitudin sapien lobortis eget.\n\n      Ut at odio id nisi "
	        "malesuada varius ut ac mauris. Orci\n      varius natoque "
	        "penatibus et magnis dis parturient\n      montes, nascetur "
	        "ridiculus mus. Duis pretium pretium\n      fringilla. Nulla vitae "
	        "urna lacinia, viverra ex eu,\n      sollicitudin lorem. "
	        "Vestibulum ut elit consectetur,\n      placerat turpis vulputate, "
	        "malesuada dui. Nulla\n      sagittis nisi ut luctus cursus. "
	        "Quisque sodales sapien\n      quis tempor efficitur. Praesent sit "
	        "amet mi ac erat\n      dictum ultrices. Ut arcu nibh, blandit "
	        "vitae nisi sed,\n      sollicitudin bibendum ligula."
	        ""sv,
	        {.commit = sys_seconds{951827696s}},
	    },
	    {
	        "mid printout"sv,
	        "%hr %pP (%pr) - [%rD] %an <%ae>%n%n%w(54,5,8)%B"sv,
	        "112233445 100% (pass) - [develop] Johnny Appleseed "
	        "<johnny@appleseed.com>\n\n     Subject, isn't it?\n\n        "
	        "Lorem ipsum dolor sit amet, consectetur\n        adipiscing elit. "
	        "Praesent facilisis feugiat\n        nibh in sodales. Nullam non "
	        "velit lectus.\n        Morbi finibus risus vel rutrum faucibus. "
	        "Fusce\n        commodo ultrices blandit. Nulla congue\n        "
	        "accumsan risus, ut tempus lorem facilisis\n        quis. Nunc "
	        "ultricies metus sed lacinia\n        lacinia. Aenean eu euismod "
	        "purus, sed pulvinar\n        libero. Sed a nibh sed tortor "
	        "venenatis\n        faucibus ac sit amet massa. Sed in fringilla\n "
	        "       ante. Integer tristique vulputate nulla nec\n        "
	        "tristique. Ut finibus leo ut lacinia molestie.\n        Mauris "
	        "blandit tortor augue, nec fermentum\n        augue varius in. Nam "
	        "ut ultrices enim. Nulla\n        facilisi. Phasellus auctor et "
	        "arcu vel\n        molestie. Proin accumsan rutrum risus, vel\n    "
	        "    sollicitudin sapien lobortis eget.\n\n        Ut at odio id "
	        "nisi malesuada varius ut ac\n        mauris. Orci varius natoque "
	        "penatibus et\n        magnis dis parturient montes, nascetur\n    "
	        "    ridiculus mus. Duis pretium pretium fringilla.\n        Nulla "
	        "vitae urna lacinia, viverra ex eu,\n        sollicitudin lorem. "
	        "Vestibulum ut elit\n        consectetur, placerat turpis "
	        "vulputate,\n        malesuada dui. Nulla sagittis nisi ut "
	        "luctus\n        cursus. Quisque sodales sapien quis tempor\n      "
	        "  efficitur. Praesent sit amet mi ac erat dictum\n        "
	        "ultrices. Ut arcu nibh, blandit vitae nisi\n        sed, "
	        "sollicitudin bibendum ligula."
	        ""sv,
	        {.commit = sys_seconds{951827696s}},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, format, ::testing::ValuesIn(tests));

	static constexpr auto feb29 =
	    date::sys_days{date::year{2000} / date::feb / 29} + 12h + 34min + 56s;

	TEST_F(format, long_line) {
		auto fmt = formatter::from("%w(30)%b"sv);

		ph::context ctx = context({});
		git_oid id{};
		git_oid_fromstr(&id, "112233445566778899aabbccddeeff0012345678");
		auto report = make_report(
		    R"(Subject line

This-line-is-too-long-to-be-properly-wrapped. However, this line is perfectly wrappable)"s,
		    feb29, feb29, std::nullopt);
		ASSERT_TRUE(report);
		static constexpr auto expected =
		    R"(      This-line-is-too-long-to-be-properly-wrapped.
      However, this line is
      perfectly wrappable)"sv;

		auto view = ph::report_view::from(*report, &id);
		auto actual = fmt.format(view, ctx);
		ASSERT_EQ(expected, actual);
	}

	TEST_F(format, starts_excatly_at_limit) {
		auto fmt = formatter::from("%w(30, 5)%b"sv);

		ph::context ctx = context({});
		git_oid id{};
		git_oid_fromstr(&id, "112233445566778899aabbccddeeff0012345678");
		auto report = make_report(
		    R"(Subject line

1234567890123456789012345 1234 56789 1234567890 987
5643 21.)"s,
		    feb29, feb29, std::nullopt);
		ASSERT_TRUE(report);
		static constexpr auto expected =
		    R"(     1234567890123456789012345
     1234 56789 1234567890 987
     5643 21.)"sv;

		auto view = ph::report_view::from(*report, &id);
		auto actual = fmt.format(view, ctx);
		ASSERT_EQ(expected, actual);
	}
}  // namespace cov::testing
