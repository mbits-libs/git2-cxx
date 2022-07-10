// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/branch.hh>
#include <cov/io/file.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
#include <git2/config.hh>
#include <git2/global.hh>
#include "path-utils.hh"
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	// using namespace std::filesystem;

	class repository : public ::testing::Test {
	public:
		void run_setup(std::vector<path_info> const& steps) {
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}
	};

	TEST_F(repository, bad_discovery) {
		git::init init{};

		run_setup(
		    make_setup(remove_all("repository"sv),
		               create_directories("repository/discover/subdir"sv)));

		auto const covdir = cov::repository::discover(
		    setup::test_dir() / "repository/discover/subdir"sv);
		ASSERT_TRUE(covdir.empty());
	}

	TEST_F(repository, good_discovery) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_repo("repository/actual"sv),
		    touch("repository/discover/.covdata"sv,
		          "covdata: \t ../actual \t \n"sv),
		    create_directories("repository/discover/subdir"sv)));

		auto const covdir = cov::repository::discover(
		    setup::test_dir() / "repository/discover/subdir"sv);
		ASSERT_EQ(setup::get_path(setup::test_dir() / "repository/actual"sv),
		          setup::get_path(covdir));
	}

	TEST_F(repository, init) {
		git::init init{};

		run_setup(make_setup(remove_all("iniside_git"sv),
		                     create_directories("iniside_git/.git"sv)));

		std::error_code ec{};
		auto const repo = cov::repository::init(
		    setup::test_dir() / "iniside_git/.git/.covdata"sv,
		    setup::test_dir() / "iniside_git/.git"sv, ec);
		ASSERT_FALSE(ec);

		auto const& commondir = repo.commondir();
		std::vector<entry_t> entries;
		for (auto const& entry : recursive_directory_iterator{commondir}) {
			entries.push_back(
			    {setup::get_path(proximate(entry.path(), commondir)),
			     entry.status().type()});
		}
		std::sort(entries.begin(), entries.end());

		std::vector<entry_t> const expected_tree{
		    {"HEAD"s, file_type::regular},
		    {"config"s, file_type::regular},
		    {"objects"s, file_type::directory},
		    {"objects/coverage"s, file_type::directory},
		    {"objects/pack"s, file_type::directory},
		    {"refs"s, file_type::directory},
		    {"refs/heads"s, file_type::directory},
		    {"refs/tags"s, file_type::directory},
		};

		ASSERT_EQ(expected_tree, entries);

		static constexpr auto gitdir = ".."sv;
		auto dir = repo.config().get_string("core.gitdir");
		ASSERT_TRUE(dir);
		ASSERT_EQ(gitdir, *dir);
	}

	TEST_F(repository, no_reports) {
		git::init init{};

		run_setup(make_setup(remove_all("repository"sv),
		                     init_repo("repository/.covdata"sv)));

		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv);
		ASSERT_EQ(setup::get_path(setup::test_dir() / "repository/.covdata"sv),
		          setup::get_path(covdir));

		std::error_code ec{};
		auto const repo = cov::repository::open(covdir, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo.refs());

		auto const HEAD = repo.refs()->lookup("HEAD"sv);
		ASSERT_TRUE(HEAD);
		ASSERT_EQ(reference_type::symbolic, HEAD->reference_type());

		auto const empty_repo = !HEAD->peel_target();
		ASSERT_TRUE(empty_repo);
	}

	TEST_F(repository, detached) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_repo("repository/.covdata"sv),
		    touch("repository/.covdata/HEAD"sv,
		          "f8e21b2735744fa148e8c6e0bb59d7629857d2a7\n"sv)));

		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv);
		ASSERT_EQ(setup::get_path(setup::test_dir() / "repository/.covdata"sv),
		          setup::get_path(covdir));

		std::error_code ec{};
		auto const repo = cov::repository::open(covdir, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo.refs());

		auto const HEAD = repo.refs()->lookup("HEAD"sv);
		ASSERT_TRUE(HEAD);
		ASSERT_EQ(reference_type::direct, HEAD->reference_type());
	}

	TEST_F(repository, dwim) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_repo("repository/.covdata"sv),
		    touch("repository/.covdata/refs/heads/main"sv,
		          "f8e21b2735744fa148e8c6e0bb59d7629857d2a7\n"sv),
		    touch("repository/.covdata/refs/tags/v1.0.0"sv,
		          "cb62947f3ebcaf2869b0a7c1c76673db3641b72a\n"sv)));

		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv);
		ASSERT_EQ(setup::get_path(setup::test_dir() / "repository/.covdata"sv),
		          setup::get_path(covdir));

		std::error_code ec{};
		auto const repo = cov::repository::open(covdir, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo.refs());

		auto const HEAD = as_a<cov::reference>(repo.dwim("HEAD"sv));
		ASSERT_TRUE(HEAD);
		auto const peeled = HEAD->peel_target();
		ASSERT_TRUE(peeled);
		ASSERT_EQ("main"sv, peeled->shorthand());
		ASSERT_TRUE(peeled->references_branch());

		auto const tag = as_a<cov::tag>(repo.dwim("tags/v1.0.0"sv));
		ASSERT_TRUE(tag);
		ASSERT_EQ("v1.0.0"sv, tag->name());

		auto const branch = as_a<cov::branch>(repo.dwim("heads/main"sv));
		ASSERT_TRUE(branch);
		ASSERT_EQ("main"sv, branch->name());
	}
}  // namespace cov::testing
