// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/reference.hh>
#include <variant>
#include "path-utils.hh"
#include "setup.hh"

using namespace git::literals;

namespace cov::testing {
	using namespace std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class REF { branch, tag, neither };

	struct refs_test {
		std::string_view name;
		std::string_view ref_name;
		std::string_view prev_content;
		std::variant<git::oid, std::string_view> target;
		std::variant<git::oid, std::string_view> expectation;
		std::string_view expected{};
		std::string_view expected_matching{};

		friend std::ostream& operator<<(std::ostream& out,
		                                refs_test const& param) {
			return out << param.name;
		}
	};

	class refs : public TestWithParam<refs_test> {};

	TEST_P(refs, create) {
		auto const& [name, ref_name, __, target, expectation, expected, _] =
		    GetParam();

		{
			std::error_code ec{};
			path_info::op(
			    make_setup(path_info{name, path_kind::remove_all},
			               path_info{name, path_kind::create_directories}),
			    ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs = cov::references::make_refs(setup::test_dir() / name);
		ASSERT_TRUE(refs);

		auto result = std::visit(
		    [refs, ref_name = ref_name](auto const& tgt) {
			    return refs->create(ref_name, tgt);
		    },
		    target);

		if (expected.empty()) {
			ASSERT_FALSE(result);
			return;
		}

		ASSERT_TRUE(result);

		auto in =
		    io::fopen(setup::test_dir() / name / setup::make_u8path(ref_name));
		auto actual = in.read_line();

		ASSERT_EQ(expected, actual);
	}

	TEST_P(refs, create_matching) {
		auto const& [name, ref_name, prev_content, target, expectation, _,
		             expected] = GetParam();

		{
			std::error_code ec{};
			path_info::op(
			    make_setup(path_info{name, path_kind::remove_all},
			               path_info{name, path_kind::create_directories}),
			    ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
			auto path = setup::test_dir() / name / setup::make_u8path(ref_name);
			std::filesystem::create_directories(path.parent_path());
			auto out = io::fopen(path, "wb");
			out.store(prev_content.data(), prev_content.size());
		}

		auto refs = cov::references::make_refs(setup::test_dir() / name);
		ASSERT_TRUE(refs);

		bool modified = false;
		auto result = std::visit(
		    [refs, ref_name = ref_name, &modified](auto const& tgt,
		                                           auto const& match) {
			    if constexpr (!std::is_same_v<decltype(tgt), decltype(match)>) {
				    return ref_ptr<cov::reference>{};
			    } else {
				    return refs->create_matching(ref_name, tgt, match,
				                                 modified);
			    }
		    },
		    target, expectation);

		if (expected.empty()) {
			ASSERT_FALSE(result);
			return;
		}

		ASSERT_TRUE(result);

		auto in =
		    io::fopen(setup::test_dir() / name / setup::make_u8path(ref_name));
		auto actual = in.read_line();

		ASSERT_EQ(expected, actual);
	}

	static constexpr auto ID_1 = git::oid{git_oid{
	    "\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff\x12\x34"
	    "\x56\x78"}};
	static constexpr auto ID_2 = git::oid{git_oid{
	    "\x22\x11\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff\x12"
	    "\x34"
	    "\x56\x78"}};
	static constexpr auto ID_text =
	    "112233445566778899aabbccddeeff1234567800\n"sv;
	static constexpr auto ID_exp = "112233445566778899aabbccddeeff1234567800"sv;

	static constexpr refs_test const tests[] = {
	    {"empty"sv, {}, {}, ""sv, ""sv},
	    {
	        "OID-good"sv,
	        "refs/heads/name"sv,
	        ID_text,
	        ID_1,
	        ID_1,
	        ID_exp,
	        ID_exp,
	    },
	    {
	        .name = "OID-bad"sv,
	        .ref_name = "refs/heads/name"sv,
	        .prev_content = ID_text,
	        .target = ID_1,
	        .expectation = ID_2,
	        .expected = ID_exp,
	        .expected_matching = {},
	    },
	    {
	        "symlink-good"sv,
	        "HEAD"sv,
	        "ref: refs/heads/main\n"sv,
	        "refs/symlink"sv,
	        "refs/heads/main"sv,
	        "ref: refs/symlink"sv,
	        "ref: refs/symlink"sv,
	    },
	    {
	        "symlink-bad"sv,
	        "HEAD"sv,
	        "ref: refs/symlink\n"sv,
	        "refs/symlink"sv,
	        "refs/heads/main"sv,
	        "ref: refs/symlink"sv,
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, refs, ::testing::ValuesIn(tests));
}  // namespace cov::testing
