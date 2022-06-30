// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/reference.hh>
#include <variant>
#include "path-utils.hh"
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class REF { branch, tag, neither };

	struct references_dwim_test {
		std::string_view name;
		std::string_view shorthand;
		std::string_view expected;
		std::vector<path_info> steps{};
		REF type{REF::neither};

		friend std::ostream& operator<<(std::ostream& out,
		                                references_dwim_test const& param) {
			return out << param.name;
		}
	};

	class dwim : public TestWithParam<references_dwim_test> {};

	TEST_P(dwim, lookup) {
		auto const& [_, shorthand, expected, steps, type] = GetParam();

		{
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / steps.front().name);
		ASSERT_TRUE(refs);

		auto ref = refs->dwim(shorthand);

		if (expected.empty()) {
			ASSERT_FALSE(ref);
			return;
		}

		ASSERT_TRUE(ref);

		ASSERT_EQ(type == REF::branch, ref->is_branch());
		ASSERT_EQ(type == REF::tag, ref->is_tag());
		ASSERT_EQ(expected, ref->name());
	}

	static constexpr auto ID = "112233445566778899aabbccddeeff0012345678"sv;
	static constexpr auto ID_ = "112233445566778899aabbccddeeff0012345678\n"sv;

	static references_dwim_test const tests[] = {
	    {
	        "empty"sv,
	        {},
	        {},
	        make_setup(remove_all("empty"sv), create_directories("empty"sv)),
	    },
	    {
	        "branch"sv,
	        "name-1"sv,
	        "refs/heads/name-1"sv,
	        make_setup(remove_all("branch"sv),
	                   create_directories("branch"sv),
	                   touch("branch/refs/heads/name-1"sv, ID_),
	                   touch("branch/refs/tags/name-2"sv, ID_)),
	        REF::branch,
	    },
	    {
	        "tags-before-branches"sv,
	        "name"sv,
	        "refs/tags/name"sv,
	        make_setup(remove_all("tags-before-branches"sv),
	                   create_directories("tags-before-branches"sv),
	                   touch("tags-before-branches/refs/heads/name"sv, ID_),
	                   touch("tags-before-branches/refs/tags/name"sv, ID_)),
	        REF::tag,
	    },
	    {
	        "direct-before-branches"sv,
	        "name"sv,
	        "refs/name"sv,
	        make_setup(remove_all("direct-before-branches"sv),
	                   create_directories("direct-before-branches"sv),
	                   touch("direct-before-branches/refs/heads/name"sv, ID_),
	                   touch("direct-before-branches/refs/name"sv, ID_)),
	    },
	    {
	        "refs-before-tags"sv,
	        "name"sv,
	        "refs/name"sv,
	        make_setup(remove_all("refs-before-tags"sv),
	                   create_directories("refs-before-tags"sv),
	                   touch("refs-before-tags/refs/tags/name"sv, ID_),
	                   touch("refs-before-tags/refs/name"sv, ID_)),
	    },
	    {
	        "HEAD-before-refs"sv,
	        "HEAD"sv,
	        "HEAD"sv,
	        make_setup(remove_all("HEAD-before-refs"sv),
	                   create_directories("HEAD-before-refs"sv),
	                   touch("HEAD-before-refs/HEAD"sv, ID_),
	                   touch("HEAD-before-refs/refs/HEAD"sv, ID_)),
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, dwim, ::testing::ValuesIn(tests));
}  // namespace cov::testing
