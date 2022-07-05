// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <git2/commit.hh>
#include <git2/config.hh>
#include "../setup.hh"
#include "new.hh"

namespace git::testing {
	using namespace ::std::literals;

	TEST(oom, object_lookup) {
		auto const repo = setup::open_repo();
		ASSERT_TRUE(repo);

		auto const obj = git::commit::lookup(repo, setup::hash::commit);
		ASSERT_TRUE(obj);
		ASSERT_EQ(setup::hash::commit, obj.strid());

		bool is_empty = false;
		{
			emulate_oom here{OOM_STR_THRESHOLD};
			is_empty = obj.strid().empty();
		}
		ASSERT_TRUE(is_empty);
	}

	TEST(oom, tree_lookup) {
		auto const repo = setup::open_repo();
		ASSERT_TRUE(repo);
		auto const dir = git::tree::lookup(repo, setup::hash::tree);
		ASSERT_TRUE(dir);
		ASSERT_EQ(1ull, dir.count());
		auto const readme = dir.entry_byindex(0);
		ASSERT_TRUE(readme);

		auto const readme_id = dir.entry_bypath("README.md");
		ASSERT_TRUE(readme_id);
		ASSERT_EQ(setup::hash::README, readme_id.strid());

		bool is_empty = false;
		{
			emulate_oom here{OOM_STR_THRESHOLD};
			is_empty = readme_id.strid().empty();
		}
		ASSERT_TRUE(is_empty);
	}

	TEST(oom, config_string) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.path");
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string("my.value",
		               "very/long/long/long/long/long/long/long/long/long/long/"
		               "long/long string"s);

		std::optional<std::string> actual;
		{
			emulate_oom here{OOM_STR_THRESHOLD};
			actual = cfg.get_string("my.value");
		}
		ASSERT_FALSE(actual);
	}

	TEST(oom, config_path) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.path");
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_path("my.value",
		             "very/long/long/long/long/long/long/long/long/long/long/"
		             "long/long string"sv);

		std::optional<std::filesystem::path> actual;
		{
			emulate_oom here{OOM_STR_THRESHOLD};
			actual = cfg.get_path("my.value");
		}
		ASSERT_FALSE(actual);
	}

	TEST(oom, repository_discover) {
		auto const start = setup::test_dir() / "gitdir/subdir/"sv;
		bool is_empty = false;
		{
			emulate_oom here{OOM_STR_THRESHOLD};
			is_empty = git::repository::discover(start).empty();
		}
		ASSERT_TRUE(is_empty);
	}
}  // namespace git::testing
