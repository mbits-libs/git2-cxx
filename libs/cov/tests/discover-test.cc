// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <concepts>
#include <cov/discover.hh>
#include <cov/init.hh>
#include <cov/io/file.hh>
#include <git2/config.hh>
#include <git2/global.hh>
#include "path-utils.hh"
#include "setup.hh"

namespace cov::testing {
	struct discover_test {
		std::string_view name;
		std::string_view location;
		std::string_view expected;
		std::vector<path_info> steps;

		friend std::ostream& operator<<(std::ostream& out,
		                                discover_test const& param) {
			return out << param.name;
		}
	};

	class discover : public ::testing::TestWithParam<discover_test> {};

	TEST_P(discover, repo) {
		auto const& [_, location, expected, steps] = GetParam();

		git::init init{};
		{
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		std::error_code ec{};
		auto const repo = setup::get_path(
		    discover_repository(setup::test_dir() / location, ec));
		if (expected.empty()) {
			ASSERT_TRUE(ec);
			ASSERT_TRUE(repo.empty());
		} else {
			ASSERT_FALSE(ec);
			ASSERT_EQ(setup::get_path(canonical(setup::test_dir() / expected)),
			          repo);
		}
	}

	static discover_test const tests[] = {
	    {
	        "empty"sv,
	        "discover"sv,
	        {},
	        make_setup(remove_all("discover"sv),
	                   create_directories("discover"sv)),
	    },
	    {
	        "current"sv,
	        "discover"sv,
	        "discover"sv,
	        make_setup(remove_all("discover"sv), init_repo("discover"sv)),
	    },
	    {
	        "covlink"sv,
	        "discover"sv,
	        "actual"sv,
	        make_setup(
	            remove_all("discover"sv),
	            remove_all("actual"sv),
	            init_repo("actual"sv),
	            touch("discover/.covdata"sv, "covdata: \t ../actual \t \n"sv)),
	    },
	    {
	        "broken_covlink"sv,
	        "discover"sv,
	        {},
	        make_setup(remove_all("discover"sv),
	                   touch("discover/.covdata"sv, "covdat"sv)),
	    },
	    {
	        "fake_covlink"sv,
	        "discover"sv,
	        {},
	        make_setup(remove_all("discover"sv),
	                   touch("discover/.covdata"sv, "CovData: ../actual\n"sv)),
	    },
	    {
	        "inside_git"sv,
	        "discover/subdir"sv,
	        "discover/.git/.covdata"sv,
	        make_setup(
	            remove_all("discover"sv),
	            create_directories("discover/subdir"sv),
	            create_directories("discover/.git/objects/pack"sv),
	            create_directories("discover/.git/objects/info"sv),
	            create_directories("discover/.git/refs/tags"sv),
	            touch("discover/.git/HEAD"sv),
	            touch("discover/.git/config"sv),
	            touch("discover/.git/refs/heads/main"sv),
	            init_repo("discover/.git/.covdata"sv, "discover/.git/"sv)),
	    },
	    {
	        "is_valid_path"sv,
	        "discover/subdir"sv,
	        {},
	        make_setup(remove_all("discover"sv),
	                   create_directories("discover/subdir/objects/coverage"sv),
	                   create_directories("discover/objects/coverage"sv),
	                   touch("discover/config")),
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, discover, ::testing::ValuesIn(tests));
}  // namespace cov::testing
