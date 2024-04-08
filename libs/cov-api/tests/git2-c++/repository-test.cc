// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/git2/commit.hh>
#include <cov/git2/repository.hh>
#include <filesystem>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	using setup::get_path;
	using setup::make_u8path;
	using std::filesystem::path;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	path append(path const& root, std::string_view utf8) {
		return root / make_u8path(utf8);
	}

	path make_absolute(std::string_view utf8, bool absolute = false) {
		if (absolute) return make_u8path(utf8);

		return append(setup::test_dir(), utf8);
	}

	enum class repo_kind { failing, bare, workspace };
	struct repo_param {
		std::string_view start_path{};
		struct {
			std::string_view discovered{};
			std::string_view workdir{};
			std::string_view head{};
			std::optional<std::string_view> common{std::nullopt};
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
		std::error_code ec{};
		auto const repo =
		    git::repository::open(make_absolute("gitdir/.git/"sv), ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);
		bool seen_bare = false;
		bool seen_something_else = false;
		std::string something_else;
		repo.submodule_foreach([&](auto const&, auto name) {
			if (name == "bare"sv) {
				seen_bare = true;
			} else {
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
		std::error_code ec{};
		auto const result = git::repository::discover(start, ec);
		if (expected.discovered == std::string_view{}) {
			ASSERT_TRUE(ec);
			ASSERT_EQ(get_path(result), std::string{});
			return;
		}

		ASSERT_FALSE(ec);
		ASSERT_EQ(get_path(result),
		          get_path(make_absolute(expected.discovered,
		                                 kind == repo_kind::failing)));
	}

	TEST_P(repository, open) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		std::error_code ec{};
		auto const result = git::repository::discover(start, ec);
		if (expected.discovered == std::string_view{}) {
			ASSERT_TRUE(ec);
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_FALSE(ec);
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}

		if (!result.empty()) {
			auto const repo = git::repository::open(result, ec);
			ASSERT_FALSE(ec);
			ASSERT_TRUE(repo);
		}
	}

	TEST_P(repository, wrap) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		std::error_code ec{};
		auto const result = git::repository::discover(start, ec);
		if (expected.discovered == std::string_view{}) {
			ASSERT_TRUE(ec);
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_FALSE(ec);
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}

		if (!result.empty()) {
			auto const db = git::odb::open(result, ec);
			ASSERT_FALSE(ec);
			auto const repo = git::repository::wrap(db, ec);
			ASSERT_FALSE(ec);
			ASSERT_TRUE(db);
			ASSERT_TRUE(repo);
		}
	}

	TEST_P(repository, workdir) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		std::error_code ec{};
		auto const result = git::repository::discover(start, ec);
		if (expected.discovered == std::string_view{}) {
			ASSERT_TRUE(ec);
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_FALSE(ec);
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}
		if (result.empty()) return;

		auto const repo = git::repository::open(result, ec);
		ASSERT_FALSE(ec);
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
		std::error_code ec{};
		auto const result = git::repository::discover(start, ec);
		if (expected.discovered == std::string_view{}) {
			ASSERT_TRUE(ec);
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_FALSE(ec);
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}
		if (result.empty()) return;

		auto const repo = git::repository::open(result, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);
		auto const common = repo.commondir();
		if (expected.common) {
			ASSERT_EQ(get_path(make_absolute(*expected.common,
			                                 kind == repo_kind::failing)),
			          common);
		} else {
			ASSERT_EQ(get_path(result), common);
		}
	}

	TEST_P(repository, head) {
		auto [start_path, expected, kind] = GetParam();
		auto const start =
		    make_absolute(start_path, kind == repo_kind::failing);
		std::error_code ec{};
		auto const result = git::repository::discover(start, ec);
		if (expected.discovered == std::string_view{}) {
			ASSERT_TRUE(ec);
			ASSERT_EQ(get_path(result), std::string{});
		} else {
			ASSERT_FALSE(ec);
			ASSERT_EQ(get_path(result),
			          get_path(make_absolute(expected.discovered,
			                                 kind == repo_kind::failing)));
		}
		if (result.empty()) return;

		auto const repo = git::repository::open(result, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo);
		auto const head = repo.head(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(head);
		auto const resolved = head.resolve(ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(resolved);

		auto const sha = [resolved = resolved.raw()] {
			char buffer[GIT_OID_HEXSZ];
			git_oid_fmt(buffer, git_reference_target(resolved));
			return std::string{buffer, GIT_OID_HEXSZ};
		}();

		ASSERT_EQ(expected.head, sha);
	}

	static constexpr auto b3 = "b3f67d63bd1c87427805eba4aa028bfa43587f78"sv;
	static constexpr auto b7 = "b78559c626ce713277ed5c9bac57e20785091000"sv;

	constexpr repo_param dirs[] = {
	    {"/"sv, {}, repo_kind::failing},
	    {"c:/"sv, {}, repo_kind::failing},
	    {"bare.git/"sv, {"bare.git/"sv, ""sv, b3}, repo_kind::bare},
	    {"bare.git"sv, {"bare.git/"sv, ""sv, b3}, repo_kind::bare},
	    {"gitdir/subdir/"sv, {"gitdir/.git/"sv, "gitdir/"sv, b7}},
	    {"gitdir/"sv, {"gitdir/.git/"sv, "gitdir/"sv, b7}},
	    {"gitdir"sv, {"gitdir/.git/"sv, "gitdir/"sv, b7}},
	    {
	        "gitdir/bare/"sv,
	        {"gitdir/.git/modules/bare/"sv, "gitdir/bare/"sv, b3},
	    },
	    {
	        "worktree/subdir/"sv,
	        {"gitdir/.git/worktrees/worktree/"sv, "worktree/"sv, b7,
	         "gitdir/.git/"sv},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(dirs, repository, ValuesIn(dirs));
}  // namespace git::testing
