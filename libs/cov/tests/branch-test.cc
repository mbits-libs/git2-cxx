// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/branch.hh>
#include <cov/tag.hh>
#include "path-utils.hh"

namespace cov::testing {
	using namespace ::std::literals;

	TEST(branch, valid_name) {
		EXPECT_TRUE(cov::branch::is_valid_name("HEAD"sv));
		EXPECT_TRUE(cov::branch::is_valid_name("v1.2.3"sv));
		EXPECT_TRUE(cov::branch::is_valid_name("domain/section"sv));
	}

	TEST(branch, invalid_name) {
		EXPECT_FALSE(cov::branch::is_valid_name(".hidden"sv));
		EXPECT_FALSE(cov::branch::is_valid_name("from..to"sv));
		EXPECT_FALSE(cov::branch::is_valid_name("domain.lock/section"sv));
		EXPECT_FALSE(cov::branch::is_valid_name("/domain/section"sv));
		EXPECT_FALSE(cov::branch::is_valid_name("domain/section/"sv));
	}

	static constexpr git_oid id = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	                                0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE,
	                                0xFF, 0x00, 0x12, 0x34, 0x56, 0x78}};
	static constexpr auto ID_ = "112233445566778899aabbccddeeff0012345678\n"sv;

	TEST(branch, lookup) {
		{
			std::error_code ec{};
			path_info::op(
			    make_setup(
			        remove_all("branch-lookup"sv),
			        create_directories("branch-lookup"sv),
			        touch("branch-lookup/refs/heads/feat/task-1"sv, ID_)),
			    ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / "branch-lookup"sv);
		ASSERT_TRUE(refs);

		auto branch = cov::branch::lookup("feat/task-1"sv, *refs);
		ASSERT_TRUE(branch);
		ASSERT_EQ(obj_branch, branch->type());
		ASSERT_TRUE(is_a<cov::branch>(static_cast<object*>(branch.get())));
		ASSERT_TRUE(!is_a<cov::tag>(static_cast<object*>(branch.get())));
		ASSERT_EQ(0, git_oid_cmp(&id, branch->id()));
		ASSERT_EQ("feat/task-1"sv, branch->name());
	}

	TEST(branch, create) {
		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("branch-create"sv),
			                         create_directories("branch-create"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / "branch-create"sv);
		ASSERT_TRUE(refs);

		auto branch = cov::branch::create("feat/task-1"sv, id, *refs);
		ASSERT_TRUE(branch);
		ASSERT_EQ(0, git_oid_cmp(&id, branch->id()));
		ASSERT_EQ("feat/task-1"sv, branch->name());

		auto in = io::fopen(setup::test_dir() /
		                    "branch-create/refs/heads/feat/task-1"sv);
		auto contents = in.read();
		auto view = std::string_view{
		    reinterpret_cast<char const*>(contents.data()), contents.size()};
		ASSERT_EQ(ID_, view);
	}

	TEST(branch, iterate) {
		std::vector<std::string_view> expected{"feat/task-1"sv, "main"sv};

		{
			std::error_code ec{};
			path_info::op(
			    make_setup(
			        remove_all("branch-iterate"sv),
			        create_directories("branch-iterate"sv),
			        touch("branch-iterate/refs/heads/main"sv, ID_),
			        touch("branch-iterate/refs/heads/feat/task-1"sv, ID_),
			        touch("branch-iterate/refs/tags/moving"sv, ID_),
			        touch("branch-iterate/refs/tags/release/v1.2.3"sv, ID_)),
			    ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / "branch-iterate"sv);
		ASSERT_TRUE(refs);

		auto iter = cov::branch::iterator(*refs);
		ASSERT_TRUE(iter);
		ASSERT_EQ(obj_branch_list, iter->type());
		ASSERT_TRUE(is_a<cov::branch_list>(static_cast<object*>(iter.get())));
		ASSERT_TRUE(!is_a<cov::tag_list>(static_cast<object*>(iter.get())));

		std::vector<ref_ptr<cov::branch>> branchs{};
		std::copy(iter->begin(), iter->end(), std::back_inserter(branchs));
		std::vector<std::string_view> names{};
		names.reserve(branchs.size());
		std::transform(branchs.begin(), branchs.end(),
		               std::back_inserter(names),
		               [](auto& tag) { return tag->name(); });
		std::sort(names.begin(), names.end());
		ASSERT_EQ(expected, names);
	}
}  // namespace cov::testing
