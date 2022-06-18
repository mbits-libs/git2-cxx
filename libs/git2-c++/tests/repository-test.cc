// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <git2-c++/repository.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	using setup::get_path;
	using setup::make_path;
	using std::filesystem::path;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	path append(path const& root, std::string_view utf8) {
		return root / make_path(utf8);
	}

	path make_absolute(std::string_view utf8, bool absolute) {
		if (absolute) return make_path(utf8);

		return append(setup::test_dir(), utf8);
	}

	struct repo_param {
		std::string_view start_path{};
		std::string_view expected{};
		bool absolute{false};
	};

	class repository : public TestWithParam<repo_param> {};

	TEST_P(repository, discover) {
		auto [start_path, expected, absolute] = GetParam();
		auto const start = make_absolute(start_path, absolute);
		auto const result = git::repository::discover(start);
		if (expected == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
			return;
		}

		ASSERT_EQ(get_path(result),
		          get_path(make_absolute(expected, absolute)));
	}

	TEST_P(repository, open) {
		auto [start_path, expected, absolute] = GetParam();
		auto const start = make_absolute(start_path, absolute);
		auto const result = git::repository::discover(start);
		if (expected == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected, absolute)));
		}

		if (!result.empty()) {
			auto const repo = git::repository::open(result);
			ASSERT_TRUE(repo);
		}
	}

	TEST_P(repository, wrap) {
		auto [start_path, expected, absolute] = GetParam();
		auto const start = make_absolute(start_path, absolute);
		auto const result = git::repository::discover(start);
		if (expected == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected, absolute)));
		}

		if (!result.empty()) {
			auto const db = git::odb::open(result);
			auto const repo = git::repository::wrap(db);
			ASSERT_TRUE(db);
			ASSERT_TRUE(repo);
		}
	}

	constexpr repo_param dirs[] = {
	    {"/", {}, true},
	    {"c:/", {}, true},
	    {"bare.git/", "bare.git/"},
	    {"bare.git", "bare.git/"},
	    {"gitdir/subdir/", "gitdir/.git/"},
	    {"gitdir/", "gitdir/.git/"},
	    {"gitdir", "gitdir/.git/"},
	};

	INSTANTIATE_TEST_SUITE_P(dirs, repository, ValuesIn(dirs));
}  // namespace git::testing
