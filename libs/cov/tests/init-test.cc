// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/init.hh>
#include <cov/io/file.hh>
#include <git2/config.hh>
#include <git2/global.hh>
#include "path-utils.hh"
#include "setup.hh"

namespace cov::testing {
	struct entry_t {
		std::string filename;
		file_type type;

		bool operator==(entry_t const& other) const noexcept {
			return filename == other.filename && type == other.type;
		}

		bool operator<(entry_t const& other) const noexcept {
			if (filename == other.filename) return type < other.type;
			return filename < other.filename;
		}

		friend std::ostream& operator<<(std::ostream& out,
		                                entry_t const& entry) {
			return out << "{\"" << entry.filename << "\", "
			           << static_cast<int>(entry.type) << '}';
		}
	};

	std::optional<std::string> core_gitdir(path const& base_dir) {
		auto config = git::config::create();
		if (config.add_file_ondisk(
		        setup::get_path(base_dir / "config"sv).c_str()))
			return std::nullopt;
		return config.get_string("core.gitdir");
	}

	struct init_test {
		std::string_view name{};
		struct {
			std::string_view location{};
			std::string_view git{};
			init_options opts{};
		} repo{};
		std::vector<path_info> steps{};
		struct {
			std::string_view expected_git{};
			bool error{false};
			bool may_have_root_name{false};
		} expected{};

		friend std::ostream& operator<<(std::ostream& out,
		                                init_test const& param) {
			return out << param.name;
		}
	};

	class init : public ::testing::TestWithParam<init_test> {
	protected:
		void assert_tree(path const& result,
		                 std::string_view const& gitdir,
		                 bool may_have_root_name) {
			std::vector<entry_t> entries;
			for (auto const& entry : recursive_directory_iterator{result}) {
				entries.push_back(
				    {setup::get_path(proximate(entry.path(), result)),
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

			auto dir = core_gitdir(result);
			ASSERT_TRUE(dir);
			auto const git = setup::make_path(gitdir);
			auto const actual = setup::make_path(*dir);
			if (git != actual && may_have_root_name) {
				if (actual.has_root_name())
					*dir = setup::get_path("/" / actual.relative_path());
			}
			ASSERT_EQ(gitdir, *dir);
		}

		template <std::same_as<path_info>... PathInfo>
		void make_setup(PathInfo&&... items) {
			std::error_code ec{};
			path_info::op({items...}, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}
	};

	TEST_P(init, repo) {
		auto const& [_, repo, steps, expected] = GetParam();

		git::init init{};

		std::error_code ec{};
		path_info::op(steps, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';

		auto const result =
		    init_repository(setup::test_dir() / repo.location,
		                    setup::test_dir() / repo.git, ec, repo.opts);

		if (!expected.error) {
			ASSERT_FALSE(ec)
			    << "   Error: " << ec.message() << " (" << ec.category().name()
			    << ' ' << ec.value() << ')';
			ASSERT_EQ(setup::get_path(setup::test_dir() / repo.location),
			          setup::get_path(result));

			assert_tree(result, expected.expected_git,
			            expected.may_have_root_name);
		} else {
			ASSERT_TRUE(ec);
			ASSERT_TRUE(result.empty());
		}
	}

	static init_test const tests[] = {
	    {
	        "inside_git"sv,
	        {"iniside_git/.git/.covdata"sv, "iniside_git/.git"sv},
	        make_setup(remove_all("iniside_git"sv),
	                   create_directories("iniside_git/.git"sv)),
	        {".."sv},
	    },

	    {
	        "outside_git"sv,
	        {
	            "outside_git/.covdata"sv,
	            "/1234567890aswedferckarek/project/.git"sv,
	        },
	        make_setup(remove_all("outside_git"sv),
	                   create_directories("outside_git"sv)),
	        {
	            .expected_git = "/1234567890aswedferckarek/project/.git"sv,
	            .may_have_root_name = true,
	        },
	    },

#ifdef WIN32
	    {
	        "outside_git_win32"sv,
	        {
	            "outside_git/.covdata"sv,
	            "Z:/1234567890aswedferckarek/project/.git"sv,
	        },
	        make_setup(remove_all("outside_git"sv),
	                   create_directories("outside_git"sv)),
	        {"Z:/1234567890aswedferckarek/project/.git"sv},
	    },
#endif

	    {
	        "beside_git"sv,
	        {"beside_git/.covdata"sv, "beside_git/project/.git"sv},
	        make_setup(remove_all("beside_git"sv),
	                   create_directories("beside_git"sv)),
	        {"../project/.git"sv},
	    },

	    {
	        "preexisting"sv,
	        {"preexisting"sv},
	        make_setup(remove_all("preexisting"sv),
	                   create_directories("preexisting/objects/coverage"sv),
	                   touch("preexisting/config"sv),
	                   touch("preexisting/HEAD"sv)),
	        {.error = true},
	    },

	    {
	        "preexisting_reinit"sv,
	        {"preexisting"sv, {}, {.flags = init_options::reinit}},
	        make_setup(remove_all("preexisting"sv),
	                   create_directories("preexisting/objects/coverage"sv),
	                   touch("preexisting/config"sv),
	                   touch("preexisting/HEAD"sv)),
	        {".."sv},
	    },

	    {
	        "with_cleanup"sv,
	        {"with_cleanup"sv, {}, {.flags = init_options::reinit}},
	        make_setup(remove_all("with_cleanup"sv),
	                   create_directories("with_cleanup/objects"sv),
	                   touch("with_cleanup/objects/pack"sv)),
	        {.error = true},
	    },

	    {
	        "with_cleanup_2"sv,
	        {"with_cleanup"sv, {}, {.flags = init_options::reinit}},
	        make_setup(remove_all("with_cleanup"sv),
	                   create_directories("with_cleanup/objects"sv),
	                   create_directories("with_cleanup/HEAD"sv)),
	        {.error = true},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, init, ::testing::ValuesIn(tests));
}  // namespace cov::testing
