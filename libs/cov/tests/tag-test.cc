// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/branch.hh>
#include <cov/tag.hh>
#include "path-utils.hh"

namespace cov::testing {
	using namespace ::std::literals;

	TEST(tag, valid_name) {
		EXPECT_TRUE(cov::tag::is_valid_name("HEAD"sv));
		EXPECT_TRUE(cov::tag::is_valid_name("v1.2.3"sv));
		EXPECT_TRUE(cov::tag::is_valid_name("domain/section"sv));
	}

	TEST(tag, invalid_name) {
		EXPECT_FALSE(cov::tag::is_valid_name(".hidden"sv));
		EXPECT_FALSE(cov::tag::is_valid_name("from..to"sv));
		EXPECT_FALSE(cov::tag::is_valid_name("domain.lock/section"sv));
		EXPECT_FALSE(cov::tag::is_valid_name("/domain/section"sv));
		EXPECT_FALSE(cov::tag::is_valid_name("domain/section/"sv));
	}

	static constexpr git_oid id = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	                                0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE,
	                                0xFF, 0x00, 0x12, 0x34, 0x56, 0x78}};
	static constexpr auto ID_ = "112233445566778899aabbccddeeff0012345678\n"sv;

	TEST(tag, lookup) {
		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("tag-lookup"sv),
			                         create_directories("tag-lookup"sv),
			                         touch("tag-lookup/refs/heads/main"sv, ID_),
			                         touch("tag-lookup/refs/tags/moving"sv,
			                               "ref: refs/heads/main\n")),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / "tag-lookup"sv);
		ASSERT_TRUE(refs);

		auto tag = cov::tag::lookup("moving"sv, *refs);
		ASSERT_TRUE(tag);
		ASSERT_EQ(obj_tag, tag->type());
		ASSERT_TRUE(is_a<cov::tag>(static_cast<object*>(tag.get())));
		ASSERT_TRUE(!is_a<cov::branch>(static_cast<object*>(tag.get())));
		ASSERT_EQ(0, git_oid_cmp(&id, tag->id()));
		ASSERT_EQ("moving"sv, tag->name());
	}

	TEST(tag, create) {
		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("tag-create"sv),
			                         create_directories("tag-create"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / "tag-create"sv);
		ASSERT_TRUE(refs);

		auto tag = cov::tag::create("release/v1.2.3"sv, id, *refs);
		ASSERT_TRUE(tag);
		ASSERT_EQ(0, git_oid_cmp(&id, tag->id()));
		ASSERT_EQ("release/v1.2.3"sv, tag->name());

		auto in = io::fopen(setup::test_dir() /
		                    "tag-create/refs/tags/release/v1.2.3"sv);
		auto contents = in.read();
		auto view = std::string_view{
		    reinterpret_cast<char const*>(contents.data()), contents.size()};
		ASSERT_EQ(ID_, view);
	}

	TEST(tag, iterate) {
		std::vector<std::string_view> expected{"moving"sv, "release/v1.2.3"sv};

		{
			std::error_code ec{};
			path_info::op(
			    make_setup(
			        remove_all("tag-iterate"sv),
			        create_directories("tag-iterate"sv),
			        touch("tag-iterate/refs/heads/main"sv, ID_),
			        touch("tag-iterate/refs/heads/feat/task-1"sv, ID_),
			        touch("tag-iterate/refs/tags/moving"sv, ID_),
			        touch("tag-iterate/refs/tags/release/v1.2.3"sv, ID_)),
			    ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / "tag-iterate"sv);
		ASSERT_TRUE(refs);

		auto iter = cov::tag::iterator(*refs);
		ASSERT_TRUE(iter);
		ASSERT_EQ(obj_tag_list, iter->type());
		ASSERT_TRUE(is_a<cov::tag_list>(static_cast<object*>(iter.get())));
		ASSERT_TRUE(!is_a<cov::branch_list>(static_cast<object*>(iter.get())));

		std::vector<ref_ptr<cov::tag>> tags{};
		std::copy(iter->begin(), iter->end(), std::back_inserter(tags));
		std::vector<std::string_view> names{};
		names.reserve(tags.size());
		std::transform(tags.begin(), tags.end(), std::back_inserter(names),
		               [](auto& tag) { return tag->name(); });
		std::sort(names.begin(), names.end());
		ASSERT_EQ(expected, names);
	}
}  // namespace cov::testing
