// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include <cov/git2/global.hh>
#include <cov/repository.hh>
#include "../setup.hh"

using namespace std::literals;
using namespace git::literals;

namespace cov::placeholder::testing {
	using namespace cov::testing;
	TEST(facade, builds) {
		auto const builds = cov::report::builder{}
		                        .add_nfo({.props = R"("has_stats":true)",
		                                  .stats{
		                                      .lines_total = 1,
		                                      .lines = {1, 1},
		                                      .functions = {1, 1},
		                                      .branches = {1, 1},
		                                  }})
		                        .add_nfo({.props = R"("has_stats":false)",
		                                  .stats{
		                                      .lines_total = 0,
		                                      .lines = {0, 0},
		                                      .functions = {0, 0},
		                                      .branches = {0, 0},
		                                  }})
		                        .release();
		for (auto const& tested : builds) {
			auto const has_stats = cov::report::build::properties(
			                           tested->props_json())["has_stats"] ==
			                       cov::report::property{true};
			auto const facade =
			    object_facade::present_build(tested.get(), nullptr);
			ASSERT_TRUE(facade->condition({}));
			ASSERT_TRUE(facade->condition("prop"sv));
			ASSERT_FALSE(facade->condition("floaty mcfloat face"sv));
			// no repo behind, no file list:
			ASSERT_FALSE(facade->secondary_id());
			// 3ary and 4ary have no meaning here:
			ASSERT_FALSE(facade->tertiary_id());
			ASSERT_FALSE(facade->quaternary_id());
			// git commit is only attached to reports:
			ASSERT_FALSE(facade->git());
			// no repo behind, no added:
			ASSERT_EQ(std::chrono::sys_seconds{}, facade->added());

			ASSERT_EQ(has_stats, facade->condition("pL"sv));
			ASSERT_EQ(has_stats, facade->condition("pTL"sv));
			ASSERT_EQ(has_stats, facade->condition("pTF"sv));
			ASSERT_EQ(has_stats, facade->condition("pTB"sv));
		}

		// and now again, with no backing this time:
		auto const facade = object_facade::present_build(nullptr, nullptr);
		ASSERT_FALSE(facade->condition({}));
		ASSERT_FALSE(facade->condition("prop"sv));
		ASSERT_FALSE(facade->condition("floaty mcfloat face"sv));
		ASSERT_FALSE(facade->condition("pL"sv));
		ASSERT_FALSE(facade->condition("pTL"sv));
		ASSERT_FALSE(facade->condition("pTF"sv));
		ASSERT_FALSE(facade->condition("pTB"sv));
		ASSERT_FALSE(facade->secondary_id());
		ASSERT_FALSE(facade->tertiary_id());
		ASSERT_FALSE(facade->quaternary_id());
		ASSERT_FALSE(facade->git());
		ASSERT_EQ(std::chrono::sys_seconds{}, facade->added());
	}

	TEST(facade, report) {
		auto const report_id = "92b91e27801bbd1ee4e2cc456b81be767a03fbbf"_oid,
		           parent_id = "8765432100ffeeddccbbaa998877665544332211"_oid,
		           commit_id = "36109a1c35e0d5cf3e5e68d896c8b1b4be565525"_oid,
		           files_id = "0f0af157e8897ce4b29a524d9ef9fe75fc942ab4"_oid,
		           build1_id = "91ec54e2634abb436583b0e27ceda44c9ae5820c"_oid,
		           build2_id = "830c717a98fcd696771101d2ba9d76b45f3791b3"_oid;
		git::oid zero{};

		io::v1::coverage_stats const stats{1250, {300, 299}, {1, 1}, {0, 0}};

		using namespace date;
		static constexpr auto feb29 =
		    sys_days{2000_y / feb / 29} + 12h + 34min + 56s;

		auto report = cov::report::create(
		    report_id, parent_id, files_id, commit_id, "develop"sv,
		    {"Johnny Appleseed"sv, "johnny@appleseed.com"sv},
		    {"Johnny Committer"sv, "committer@appleseed.com"sv},
		    "Subject, isn't it?\n\nBody para 1\n\nBody para 2\n"sv, feb29,
		    feb29, stats,
		    cov::report::builder{}
		        .add_nfo({.build_id = build1_id,
		                  .props = R"("name":"build-1")",
		                  .stats = stats})
		        .add_nfo({.build_id = build2_id,
		                  .props = R"("name":"build-2")",
		                  .stats = stats})
		        .release());
		auto report_facade =
		    object_facade::present_report(report.get(), nullptr);

		ASSERT_FALSE(report_facade->condition({}));
		ASSERT_FALSE(report_facade->condition("prop"sv));
		ASSERT_FALSE(report_facade->condition("floaty mcfloat face"sv));
		ASSERT_TRUE(report_facade->condition("B"sv));
		ASSERT_TRUE(report_facade->condition("build"sv));
		ASSERT_TRUE(report_facade->condition("pL"sv));
		ASSERT_TRUE(report_facade->condition("pTL"sv));
		ASSERT_TRUE(report_facade->condition("pTF"sv));
		ASSERT_FALSE(report_facade->condition("pTB"sv));
		ASSERT_TRUE(report_facade->primary_id());
		ASSERT_TRUE(report_facade->secondary_id());
		ASSERT_TRUE(report_facade->tertiary_id());
		ASSERT_TRUE(report_facade->quaternary_id());
		ASSERT_EQ(report_id, *report_facade->primary_id());
		ASSERT_EQ(files_id, *report_facade->secondary_id());
		ASSERT_EQ(parent_id, *report_facade->tertiary_id());
		ASSERT_EQ(commit_id, *report_facade->quaternary_id());
		ASSERT_TRUE(report_facade->git());
		auto const& git = *report_facade->git();
		ASSERT_EQ("develop"sv, git.branch);
		ASSERT_EQ(
		    "Subject, isn't it?\n"
		    "\n"
		    "Body para 1\n"
		    "\n"
		    "Body para 2\n"sv,
		    git.message);
		ASSERT_EQ("johnny@appleseed.com"sv, git.author.email);
		ASSERT_EQ("Johnny Appleseed"sv, git.author.name);
		ASSERT_EQ("committer@appleseed.com"sv, git.committer.email);
		ASSERT_EQ("Johnny Committer"sv, git.committer.name);
		ASSERT_EQ(feb29, report_facade->added());

		auto const loop = report_facade->loop("B"sv);
		ASSERT_TRUE(loop);
		size_t count = 0;
		for (auto next = loop->next(); next; next = loop->next()) {
			++count;
			auto id = next->primary_id();
			ASSERT_TRUE(id);
			if (*id == build1_id) {
				ASSERT_EQ(build1_id, *id);
			} else {
				ASSERT_EQ(build2_id, *id);
			}
		}
		ASSERT_EQ(2, count);
	}

	TEST(facade, backed_by_repo_data_broken) {
		git::init app{};
		std::error_code ec{};
		std::filesystem::remove_all(setup::test_dir() / "clean/.git/.covdata"sv,
		                            ec);
		{
			git::repository::init(setup::test_dir() / "clean/.git"sv, false,
			                      ec);
		}
		auto repo = cov::repository::init(
		    setup::test_dir(), setup::test_dir() / "clean/.git/.covdata"sv,
		    setup::test_dir() / "clean/.git"sv, ec);
		ASSERT_FALSE(ec) << ec.message();

		auto const report_id = "92b91e27801bbd1ee4e2cc456b81be767a03fbbf"_oid,
		           build_id = "91ec54e2634abb436583b0e27ceda44c9ae5820c"_oid;
		git::oid zero{};

		io::v1::coverage_stats const stats{1250, {300, 299}, {1, 1}, {0, 0}};

		using namespace date;
		static constexpr auto feb29 =
		    sys_days{2000_y / feb / 29} + 12h + 34min + 56s;

		auto report = cov::report::create(
		    report_id, zero, zero, zero, "develop"sv,
		    {"Johnny Appleseed"sv, "johnny@appleseed.com"sv},
		    {"Johnny Committer"sv, "committer@appleseed.com"sv},
		    "Subject, isn't it?\n\nBody para 1\n\nBody para 2\n"sv, feb29,
		    feb29, stats,
		    cov::report::builder{}
		        .add_nfo({.build_id = build_id, .stats = stats})
		        .release());
		auto report_facade = object_facade::present_report(report.get(), &repo);

		ASSERT_TRUE(report_facade->primary_id());
		ASSERT_FALSE(report_facade->secondary_id());
		ASSERT_FALSE(report_facade->tertiary_id());
		ASSERT_FALSE(report_facade->quaternary_id());
		ASSERT_EQ(report_id, *report_facade->primary_id());
		ASSERT_TRUE(report_facade->git());

		auto loop = report_facade->loop("B"sv);
		ASSERT_TRUE(loop);
		size_t count = 0;
		for (auto next = loop->next(); next; next = loop->next()) {
			++count;
			ASSERT_EQ(std::chrono::sys_seconds{}, next->added());
			auto id = next->primary_id();
			ASSERT_TRUE(id);
			ASSERT_EQ(build_id, *id);
			ASSERT_FALSE(next->secondary_id());
			ASSERT_FALSE(next->condition({}));
		}
		ASSERT_EQ(1, count);
	}

	TEST(facade, backed_by_repo) {
		git::init app{};
		std::error_code ec{};
		std::filesystem::remove_all(setup::test_dir() / "clean/.git/.covdata"sv,
		                            ec);
		{
			git::repository::init(setup::test_dir() / "clean/.git"sv, false,
			                      ec);
		}
		auto repo = cov::repository::init(
		    setup::test_dir(), setup::test_dir() / "clean/.git/.covdata"sv,
		    setup::test_dir() / "clean/.git"sv, ec);
		ASSERT_FALSE(ec) << ec.message();

		auto const report_id = "92b91e27801bbd1ee4e2cc456b81be767a03fbbf"_oid;
		git::oid zero{}, build_id{};

		io::v1::coverage_stats const stats{1250, {300, 299}, {1, 1}, {0, 0}};

		using namespace date;
		static constexpr auto feb29 =
		    sys_days{2000_y / feb / 29} + 12h + 34min + 56s;

		auto build_obj =
		    cov::build::create(zero, feb29, R"("has_props": true)", stats);
		ASSERT_TRUE(repo.write(build_id, build_obj));

		auto report = cov::report::create(
		    report_id, zero, zero, zero, "develop"sv,
		    {"Johnny Appleseed"sv, "johnny@appleseed.com"sv},
		    {"Johnny Committer"sv, "committer@appleseed.com"sv},
		    "Subject, isn't it?\n\nBody para 1\n\nBody para 2\n"sv, feb29,
		    feb29, stats,
		    cov::report::builder{}
		        .add_nfo({.build_id = build_id, .stats = stats})
		        .release());
		auto report_facade = object_facade::present_report(report.get(), &repo);

		ASSERT_TRUE(report_facade->primary_id());
		ASSERT_FALSE(report_facade->secondary_id());
		ASSERT_FALSE(report_facade->tertiary_id());
		ASSERT_FALSE(report_facade->quaternary_id());
		ASSERT_EQ(report_id, *report_facade->primary_id());
		ASSERT_TRUE(report_facade->git());

		auto loop = report_facade->loop("B"sv);
		ASSERT_TRUE(loop);
		size_t count = 0;
		for (auto next = loop->next(); next; next = loop->next()) {
			++count;
			ASSERT_EQ(feb29, next->added());
			auto id = next->primary_id();
			ASSERT_TRUE(id);
			ASSERT_EQ(build_id, *id);
			ASSERT_TRUE(next->secondary_id());
			ASSERT_EQ(zero, *next->secondary_id());
			ASSERT_TRUE(next->condition({}));
		}
		ASSERT_EQ(1, count);
	}
}  // namespace cov::placeholder::testing
