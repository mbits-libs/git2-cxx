// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <git2/blob.hh>
#include <git2/commit.hh>
#include <git2/tree.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;

	template <typename ObjectType>
	void do_lookup(std::string_view hash) {
		git_oid oid;
		git_oid_fromstrn(&oid, hash.data(), hash.length());

		std::error_code ec{};
		auto const repo = setup::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);

		{
			auto const raw = ObjectType::lookup(repo, oid, ec);
			ASSERT_FALSE(ec);
			ASSERT_TRUE(raw);
			ASSERT_EQ(hash, raw.strid());
		}

		auto const obj = ObjectType::lookup(repo, hash, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(obj);
		ASSERT_EQ(hash, obj.strid());

		{ ASSERT_EQ(0, git_oid_cmp(&oid, &obj.oid())); }
	}

	TEST(lookup, nothing) {
		static constexpr auto hash = "nothing"sv;

		std::error_code ec{};
		auto const repo = setup::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);

		auto const obj = git::commit::lookup(repo, hash, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(obj);
	}

	TEST(lookup, commit_info) {
		std::error_code ec{};
		auto const repo = setup::open_repo(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);

		auto const obj = git::commit::lookup(repo, setup::hash::commit, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(obj);

		auto const author = obj.author();
		auto const committer = obj.committer();

		ASSERT_EQ("Johnny Appleseed"sv, author.name);
		ASSERT_EQ("johnny@ppleseed.com"sv, author.email);
		ASSERT_EQ(1657784056s, author.when.time_since_epoch());

		ASSERT_EQ("Committer Bot"sv, committer.name);
		ASSERT_EQ("bot@ppleseed.com"sv, committer.email);
		ASSERT_EQ(1657784107s, committer.when.time_since_epoch());

		ASSERT_EQ("Commit with author and commiter differing\n"sv,
		          std::string_view{obj.message_raw()});
	}

	TEST(lookup, commit) { do_lookup<git::commit>(setup::hash::commit); }
	TEST(lookup, tree) { do_lookup<git::tree>(setup::hash::tree); }
	TEST(lookup, blob) { do_lookup<git::blob>(setup::hash::README); }
}  // namespace git::testing
