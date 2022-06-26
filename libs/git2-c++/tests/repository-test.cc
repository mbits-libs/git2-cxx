// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <git2/commit.hh>
#include <git2/repository.hh>
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

	path make_absolute(std::string_view utf8, bool absolute = false) {
		if (absolute) return make_path(utf8);

		return append(setup::test_dir(), utf8);
	}

	enum class repo_kind { failing, bare, workspace };
	struct repo_param {
		std::string_view start_path{};
		struct {
			std::string_view discovered{};
			std::string_view workdir{};
		} expected{};
		repo_kind kind{repo_kind::workspace};

		friend std::ostream& operator<<(std::ostream& out,
		                                repo_param const& param) {
			out << '[';
			if (param.kind != repo_kind::failing) out << "$REPOS/";
			return out << param.start_path << ']';
		}
	};

	class repository : public TestWithParam<repo_param> {};

	TEST(repository, submodules) {
		auto const repo =
		    git::repository::open(make_absolute("gitdir/.git/"sv));
		ASSERT_TRUE(repo);
		bool seen_bare = false;
		bool seen_something_else = false;
		std::string something_else;
		repo.submodule_foreach([&](auto const& handle, auto name) {
			if (name == "bare"sv)
				seen_bare = true;
			else {
				if (!seen_something_else) something_else.assign(name);
				seen_something_else = true;
			}
		});

		ASSERT_TRUE(seen_bare);
		ASSERT_FALSE(seen_something_else) << "Seen: " << something_else;
	}

	TEST_P(repository, discover) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		auto const result = git::repository::discover(start);
		if (expected.discovered == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
			return;
		}

		ASSERT_EQ(get_path(result),
		          get_path(make_absolute(expected.discovered,
		                                 kind == repo_kind::failing)));
	}

	TEST_P(repository, open) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		auto const result = git::repository::discover(start);
		if (expected.discovered == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}

		if (!result.empty()) {
			auto const repo = git::repository::open(result);
			ASSERT_TRUE(repo);
		}
	}

	TEST_P(repository, wrap) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		auto const result = git::repository::discover(start);
		if (expected.discovered == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}

		if (!result.empty()) {
			auto const db = git::odb::open(result);
			auto const repo = git::repository::wrap(db);
			ASSERT_TRUE(db);
			ASSERT_TRUE(repo);
		}
	}

	TEST_P(repository, workdir) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		auto const result = git::repository::discover(start);
		if (expected.discovered == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}
		if (result.empty()) return;

		auto const repo = git::repository::open(result);
		ASSERT_TRUE(repo);
		auto const view = repo.workdir();
		if (kind == repo_kind::bare) {
			ASSERT_FALSE(view);
			return;
		}
		ASSERT_TRUE(view);
		ASSERT_EQ(*view, get_path(make_absolute(expected.workdir, false)));
	}

	TEST_P(repository, commondir) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		auto const result = git::repository::discover(start);
		if (expected.discovered == std::string_view{}) {
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}
		if (result.empty()) return;

		auto const repo = git::repository::open(result);
		ASSERT_TRUE(repo);
		auto const common = repo.commondir();
		ASSERT_EQ(get_path(result), common);
	}

	constexpr repo_param dirs[] = {
	    {"/"sv, {}, repo_kind::failing},
	    {"c:/"sv, {}, repo_kind::failing},
	    {"bare.git/"sv, {"bare.git/"sv}, repo_kind::bare},
	    {"bare.git"sv, {"bare.git/"sv}, repo_kind::bare},
	    {"gitdir/subdir/"sv, {"gitdir/.git/"sv, "gitdir/"sv}},
	    {"gitdir/"sv, {"gitdir/.git/"sv, "gitdir/"sv}},
	    {"gitdir"sv, {"gitdir/.git/"sv, "gitdir/"sv}},
	    {"gitdir/bare/"sv, {"gitdir/.git/modules/bare/"sv, "gitdir/bare/"sv}},
	};

	INSTANTIATE_TEST_SUITE_P(dirs, repository, ValuesIn(dirs));
}  // namespace git::testing
