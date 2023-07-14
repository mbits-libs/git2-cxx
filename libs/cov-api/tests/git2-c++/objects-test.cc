// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/git2/blob.hh>
#include <cov/git2/commit.hh>
#include <cov/git2/tree.hh>
#include <cstdio>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;

	TEST(objects, commit) {
		std::error_code ec{};
		auto const repo = setup::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);
		auto const initial = git::commit::lookup(repo, setup::hash::commit, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(initial);
		ASSERT_EQ(sys_seconds{1657784107s}, initial.commit_time_utc());

		{
			auto const tree = initial.tree(ec);
			ASSERT_FALSE(ec);
			ASSERT_TRUE(tree);
			ASSERT_EQ(setup::hash::tree, tree.strid());
		}
	}

	TEST(objects, tree) {
		std::error_code ec{};
		auto const repo = setup::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);
		auto const dir = git::tree::lookup(repo, setup::hash::tree, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(dir);
		ASSERT_EQ(1ull, dir.count());
		auto const readme = dir.entry_byindex(0);
		ASSERT_TRUE(readme);
		ASSERT_EQ(setup::hash::README, readme.strid());
		ASSERT_EQ("README.md"sv, readme.name());

		auto const copy = readme;
		ASSERT_TRUE(copy);
		ASSERT_EQ(setup::hash::README, copy.strid());

		auto const readme_id = dir.entry_bypath("README.md", ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(readme_id);
		ASSERT_EQ(setup::hash::README, readme_id.strid());

		auto const subdir = dir.tree_bypath("subdir", ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(subdir);
	}

	TEST(objects, blob) {
		std::error_code ec{};
		auto const repo = setup::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);
		auto const readme = git::blob::lookup(repo, setup::hash::README, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(readme);
		auto const content = readme.raw();
		auto const view = std::string_view{
		    reinterpret_cast<char const*>(content.data()), content.size()};
		ASSERT_EQ("# Testing repos\n\nChange from this commit\n"sv, view);

#if !defined(_WIN32) || !defined(CUTDOWN_OS)
		auto buf = readme.filtered("README.md", ec);
		ASSERT_FALSE(ec);
		auto const data = git::bytes{buf};
		auto const filtered_view = std::string_view{
		    reinterpret_cast<char const*>(data.data()), data.size()};
		ASSERT_EQ("# Testing repos\n\nChange from this commit\n"sv,
		          filtered_view);
		git_buf_dispose(&buf);
#endif
	}

}  // namespace git::testing
