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

	struct references_iterator_test {
		std::string_view name;
		std::vector<path_info> steps{};
		std::optional<std::string_view> prefix{std::nullopt};
		std::vector<std::string> expected{};

		friend std::ostream& operator<<(std::ostream& out,
		                                references_iterator_test const& param) {
			return out << param.name;
		}
	};

	class iterator : public TestWithParam<references_iterator_test> {};

	TEST_P(iterator, lookup) {
		auto const& [_, steps, prefix, expected] = GetParam();

		{
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto refs =
		    cov::references::make_refs(setup::test_dir() / steps.front().name);
		ASSERT_TRUE(refs);

		auto iter = prefix ? refs->iterator(*prefix) : refs->iterator();
		ASSERT_TRUE(iter);
		ASSERT_EQ(obj_reference_list, iter->type());
		ASSERT_TRUE(
		    is_a<cov::reference_list>(static_cast<object*>(iter.get())));
		ASSERT_FALSE(is_a<cov::references>(static_cast<object*>(iter.get())));

		std::vector<std::string> actual{};
		actual.reserve(expected.size());
		std::transform(iter->begin(), iter->end(), std::back_inserter(actual),
		               [](ref_ptr<cov::reference> const& item) -> std::string {
			               auto name = item->name();
			               return {name.data(), name.size()};
		               });
		std::sort(actual.begin(), actual.end());

		ASSERT_EQ(expected, actual);
	}

	static constexpr auto ref_ = "ref: refs/A\n"sv;

#define common_setup(test_name)                                             \
	make_setup(remove_all(test_name##sv), touch(test_name "/HEAD"sv, ref_), \
	           touch(test_name "/refs/heads/main"sv, ref_),                 \
	           touch(test_name "/refs/heads/develop"sv, ref_),              \
	           touch(test_name "/refs/heads/feat/task-1"sv, ref_),          \
	           touch(test_name "/refs/tags/v1"sv, ref_),                    \
	           touch(test_name "/refs/tags/v2"sv, ref_),                    \
	           touch(test_name "/refs/remotes/origin/HEAD"sv, ref_),        \
	           touch(test_name "/refs/remotes/origin/main"sv, ref_),        \
	           touch(test_name "/refs/remotes/origin/packages"sv, ref_))

	static references_iterator_test const tests[] = {
	    {
	        "all"sv,
	        common_setup("all"),
	        std::nullopt,
	        {
	            "refs/heads/develop"s,
	            "refs/heads/feat/task-1"s,
	            "refs/heads/main"s,
	            "refs/remotes/origin/HEAD"s,
	            "refs/remotes/origin/main"s,
	            "refs/remotes/origin/packages"s,
	            "refs/tags/v1"s,
	            "refs/tags/v2"s,
	        },
	    },
	    {
	        "branches"sv,
	        common_setup("branches"),
	        "refs/heads"sv,
	        {
	            "refs/heads/develop"s,
	            "refs/heads/feat/task-1"s,
	            "refs/heads/main"s,
	        },
	    },
	    {
	        "tags"sv,
	        common_setup("tags"),
	        "refs/tags"sv,
	        {
	            "refs/tags/v1"s,
	            "refs/tags/v2"s,
	        },
	    },
	    {
	        "origin"sv,
	        common_setup("origin"),
	        "refs/remotes/origin"sv,
	        {
	            "refs/remotes/origin/HEAD"s,
	            "refs/remotes/origin/main"s,
	            "refs/remotes/origin/packages"s,
	        },
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, iterator, ::testing::ValuesIn(tests));
}  // namespace cov::testing
