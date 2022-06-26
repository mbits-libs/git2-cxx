// Copyright (c) 2022 midnightBITS
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

		auto const repo = setup::open_repo();
		ASSERT_TRUE(repo);

		{
			auto const raw = ObjectType::lookup(repo, oid);
			ASSERT_TRUE(raw);
			ASSERT_EQ(hash, raw.strid());
		}

		auto const obj = ObjectType::lookup(repo, hash);
		ASSERT_TRUE(obj);
		ASSERT_EQ(hash, obj.strid());

		{ ASSERT_EQ(0, git_oid_cmp(&oid, &obj.oid())); }
	}

	TEST(lookup, commit) { do_lookup<git::commit>(setup::hash::commit); }
	TEST(lookup, tree) { do_lookup<git::tree>(setup::hash::tree); }
	TEST(lookup, blob) { do_lookup<git::blob>(setup::hash::README); }
}  // namespace git::testing
