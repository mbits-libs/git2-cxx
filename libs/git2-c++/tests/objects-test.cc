// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cstdio>
#include <git2-c++/blob.hh>
#include <git2-c++/commit.hh>
#include <git2-c++/tree.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;

	TEST(objects, commit) {
		auto const repo = setup::open_repo();
		ASSERT_TRUE(repo);
		auto const initial = git::commit::lookup(repo, setup::hash::commit);
		ASSERT_TRUE(initial);
		ASSERT_EQ(1655575188ull, initial.commit_time_utc());

		{
			auto const tree = initial.tree();
			ASSERT_TRUE(tree);
			ASSERT_EQ(setup::hash::tree, tree.strid());
		}
	}

	TEST(objects, tree) {
		auto const repo = setup::open_repo();
		ASSERT_TRUE(repo);
		auto const dir = git::tree::lookup(repo, setup::hash::tree);
		ASSERT_TRUE(dir);
		ASSERT_EQ(1ull, dir.count());
		auto const readme = dir.entry_byindex(0);
		ASSERT_TRUE(readme);
		ASSERT_EQ(setup::hash::README, readme.strid());
		ASSERT_EQ("README.md"sv, readme.name());

		auto const readme_id = dir.entry_bypath("README.md");
		ASSERT_TRUE(readme_id);
		ASSERT_EQ(setup::hash::README, readme_id.strid());
	}

	TEST(objects, blob) {
		auto const repo = setup::open_repo();
		ASSERT_TRUE(repo);
		auto const readme = git::blob::lookup(repo, setup::hash::README);
		ASSERT_TRUE(readme);
		auto const content = readme.raw();
		auto const view = std::string_view{
		    reinterpret_cast<char const*>(content.data()), content.size()};
		ASSERT_EQ(view, "# Testing repos\n"sv);

#if 0
		auto buf = readme.filtered("README.md");
		auto const data = git::bytes{buf};
		auto const filtered_view = std::string_view{
		    reinterpret_cast<char const*>(data.data()), data.size()};
		ASSERT_EQ(filtered_view, "# Testing repos\n"sv);
		git_buf_dispose(&buf);
#endif
	}

}  // namespace git::testing
