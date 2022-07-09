// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <fstream>
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
			emulate_oom_as_threshold here{OOM_STR_THRESHOLD};
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
			emulate_oom_as_threshold here{OOM_STR_THRESHOLD};
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
			emulate_oom_as_threshold here{OOM_STR_THRESHOLD};
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
			emulate_oom_as_threshold here{OOM_STR_THRESHOLD};
			actual = cfg.get_path("my.value");
		}
		ASSERT_FALSE(actual);
	}

#ifndef _WIN32
	TEST(oom, config_default) {
		auto const home = setup::test_dir() / "home"sv;

		unsetenv("USERPROFILE");
		unsetenv("ProgramData");
		unsetenv("XDG_CONFIG_HOME");
		setenv("HOME", home.c_str(), 1);

		{
			static constexpr auto from_home_xdg_config =
			    "[test]\nvalue=from home/.config config"sv;
			create_directories(home / ".config/dot"sv);
			std::ofstream out{home / ".config/dot/config"sv};
			out.write(
			    from_home_xdg_config.data(),
			    static_cast<std::streamsize>(from_home_xdg_config.size()));
		}

		std::error_code ignore{};
		OOM_BEGIN(100)
		auto const cfg =
		    git::config::open_default(".dotfile"sv, "dot"sv, ignore);
		OOM_END
	}
#endif

	TEST(oom, repository_discover) {
		auto const start = setup::test_dir() / "gitdir/subdir/"sv;
		bool is_empty = false;
		{
			emulate_oom_as_threshold here{OOM_STR_THRESHOLD};
			is_empty = git::repository::discover(start).empty();
		}
		ASSERT_TRUE(is_empty);
	}
}  // namespace git::testing
