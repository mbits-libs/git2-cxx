// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <git2/commit.hh>
#include <map>
#include <string>
#include <vector>
#include "renames.hh"
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	// Initial commit
	static constexpr auto old_commit =
	    "c4a21c57f65652474ae8945f84552e9e9d7c14f3"sv;
	// Go crazy
	static constexpr auto new_commit =
	    "4e9be2749a866125dac0bb7fbe26c5a1ca667aac"sv;

	TEST(diff, tree2tree) {
		std::error_code ec{};
		auto repo = renames::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);

		auto older = commit::lookup(repo, old_commit, ec);
		ASSERT_FALSE(ec);
		auto newer = commit::lookup(repo, new_commit, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(older) << "     SHA: " << old_commit;
		ASSERT_TRUE(newer) << "     SHA: " << new_commit;

		auto old_tree = older.tree(ec);
		ASSERT_FALSE(ec);
		auto new_tree = newer.tree(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(old_tree);
		ASSERT_TRUE(new_tree);

		auto diff = old_tree.diff_to(new_tree, repo, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(diff);
		ASSERT_FALSE(diff.find_similar());

		std::map<std::string, std::string> renames{};

		for (auto const& delta : diff.deltas()) {
			if (delta.status != GIT_DELTA_RENAMED &&
			    delta.status != GIT_DELTA_COPIED) {
				fmt::print("{}    {}\n", git_diff_status_char(delta.status),
				           delta.new_file.path);
				continue;
			}
			fmt::print("{}{:<3} {} {}\n", git_diff_status_char(delta.status),
			           delta.similarity, delta.new_file.path,
			           delta.old_file.path);
			renames[delta.new_file.path] = delta.old_file.path;
		}

		std::vector<std::pair<std::string, std::string>> actual{renames.begin(),
		                                                        renames.end()};

		for (auto const& [new_name, old_name] : actual) {
			fmt::print("> {} was {}\n", new_name, old_name);
		}

		std::vector<std::pair<std::string, std::string>> expected{
		    {"subfolder/file.txt"s, "file.txt"s},
		};
		ASSERT_EQ(expected, actual);
	}
}  // namespace git::testing
