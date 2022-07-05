// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <git2/commit.hh>
#include <map>
#include <string>
#include <vector>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	// [cov] Stress-testing allocator uses
	static constexpr auto new_commit =
	    "3a0d3e6bc19a581808ce005385437a4a276657c4"sv;
	// [git2] Stress-testing allocator uses
	static constexpr auto old_commit =
	    "cb62947f3ebcaf2869b0a7c1c76673db3641b72a"sv;

	TEST(diff, tree2tree) {
		auto repo = repository::open(repository::discover("."sv));
		ASSERT_TRUE(repo);

		auto newer = commit::lookup(repo, new_commit);
		auto older = commit::lookup(repo, old_commit);
		ASSERT_TRUE(newer);
		ASSERT_TRUE(older);

		auto new_tree = newer.tree();
		auto old_tree = older.tree();
		ASSERT_TRUE(new_tree);
		ASSERT_TRUE(old_tree);

		auto diff = old_tree.diff_to(new_tree, repo);
		ASSERT_TRUE(diff);
		ASSERT_FALSE(diff.find_similar());

		std::map<std::string, std::string> renames{};

		for (auto const& delta : diff.deltas()) {
			if (delta.status != GIT_DELTA_RENAMED &&
			    delta.status != GIT_DELTA_COPIED)
				continue;
			renames[delta.new_file.path] = delta.old_file.path;
		}

		std::vector<std::pair<std::string, std::string>> actual{renames.begin(),
		                                                        renames.end()};

		for (auto const& [new_name, old_name] : actual) {
			fmt::print("{} was {}\n", new_name, old_name);
		}

		std::vector<std::pair<std::string, std::string>> expected{
		    {"libs/git2-c++/tests/stress/new.cc"s,
		     "libs/git2-c++/tests/bad_alloc/new.cc"s},
		    {"libs/git2-c++/tests/stress/new.hh"s,
		     "libs/git2-c++/tests/bad_alloc/new.hh"s},
		    {"libs/git2-c++/tests/stress/oom.cc"s,
		     "libs/git2-c++/tests/bad_alloc/oom.cc"s}};
		ASSERT_EQ(expected, actual);
	}
}  // namespace git::testing
