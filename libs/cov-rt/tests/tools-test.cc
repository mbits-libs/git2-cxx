// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/app/dirs.hh>
#include <cov/app/tools.hh>
#include <git2/global.hh>

namespace cov::app::testing {
	using namespace ::std::literals;
	using namespace ::std::filesystem;

	class tools : public ::testing::Test {
	protected:
		void assert_eq(std::set<std::string_view> const& expected,
		               std::set<std::string> const& actual) {
			ASSERT_EQ(format(expected), format(actual));
		}

	private:
		template <typename String>
		std::string format(std::set<String> const& set) {
			std::string result{};
			size_t size = 0;
			for (auto const& str : set) {
				size += str.size() + 2;  // ""sv\n
			}
			result.reserve(size);
			for (auto const& str : set) {
				result.append(str);
				result.append(",\n"sv);
			}
			return result;
		}
	};

	TEST_F(tools, report_echo) {
		git::init globals{};
		app::tools runner{git::config::create(), {}};
		auto const sysroot = path{directory_info::build} / "elsewhere"sv;
		auto const actual = runner.list_externals(sysroot);
		assert_eq({"echo"sv}, actual);
	}

	TEST_F(tools, report_nothing) {
		git::init globals{};
		app::tools runner{git::config::create(), {}};
		auto const sysroot = path{directory_info::build} / "elsewhere still"sv;
		auto const actual = runner.list_externals(sysroot);
		assert_eq({}, actual);
	}

	TEST_F(tools, report_actual_externals) {
		git::init globals{};
		app::tools runner{git::config::create(), {}};
		// this test is built into bin/, just like cov
		auto const sysroot = app::tools::get_sysroot();
		auto const actual = runner.list_externals(sysroot);
		assert_eq(
		    {
		        // add missing externals here... (this test will fail with every
		        // new external added)
		    },
		    actual);
	}

	TEST_F(tools, report_aliases) {
		git::init globals{};

		auto const local = path{directory_info::source} /
		                   "apps/tests/data/recurse/.covdata/config"sv;

		auto cfg = git::config::create();
		cfg.add_file_ondisk(local);

		app::tools runner{std::move(cfg), {}};
		auto const actual = runner.list_aliases();
		assert_eq(
		    {
		        "first"sv,
		        "name"sv,
		        "second"sv,
		        "third"sv,
		    },
		    actual);
	}

	TEST_F(tools, report_builtins) {
		git::init globals{};

		static constexpr app::builtin_tool mock[] = {
		    {"first"sv, nullptr},  {"second"sv, nullptr}, {"third"sv, nullptr},
		    {"fourth"sv, nullptr}, {"fifth"sv, nullptr},
		};

		app::tools runner{git::config::create(), mock};
		auto const actual = runner.list_builtins();
		assert_eq(
		    {
		        "first"sv,
		        "second"sv,
		        "third"sv,
		        "fourth"sv,
		        "fifth"sv,
		    },
		    actual);
	}

	static int throwing_builtin(std::string_view tool, args::arglist) {
		std::string exc{};
		exc.assign(tool);
		throw exc;
	}

	TEST_F(tools, call_builtin) {
		git::init globals{};

		static constexpr app::builtin_tool mock[] = {
		    {"tool"sv, throwing_builtin},
		};

		str::strings<covlng> str{};
		str.setup_path_manager({});

		app::tools runner{git::config::create(), mock};
		std::string ignore;
		ASSERT_THROW(runner.handle(mock[0].name, ignore, {}, {}, str),
		             std::string);
	}
}  // namespace cov::app::testing
