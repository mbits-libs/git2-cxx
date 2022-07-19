// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <gtest/gtest.h>
#include <cov/app/args.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/tools.hh>
#include <git2/error.hh>
#include <git2/global.hh>

namespace cov::app::testing {
	using namespace ::std::literals;

	struct error_test {
		struct {
			int type{};
			char const* message{};
			int git_error{GIT_ERROR};
		} git{};
		std::error_code ec{};
		std::string_view excpected{};
	};

	class error : public ::testing::TestWithParam<error_test> {};
	TEST_P(error, with_domain) {
		auto const& [git, ec, expected] = GetParam();
		git::init globals{};

		auto copy = ec;

		if (!ec) {
			if (git.message) {
				git_error_set_str(git.type, git.message);
			} else {
				git_error_clear();
			}
			copy = git::as_error(git.git_error);
		}

		std::vector<std::string> en{"en"};
		str::args_translator<errlng> tr{
		    {platform::sys_root() / directory_info::share / "locale"sv, en}};

		parser_holder holder{{"cov config"sv, {}}, {}, tr.args()};
		auto const actual = holder.message(copy, tr);
		ASSERT_EQ(expected, actual);
	}

	static error_test const tests[] = {
	    {
	        .git = {GIT_ERROR_REGEX, "error message goes here"},
	        .excpected = "cov config: regex error: error message goes here"sv,
	    },
	    {
	        .git = {GIT_ERROR_CONFIG, "error message goes here"},
	        .excpected = "cov config: error: error message goes here"sv,
	    },
	    {
	        .git = {GIT_ERROR_INDEX, "neither regex nor config"},
	        .excpected = "cov config: error: neither regex nor config"sv,
	    },
	    {
	        .ec = std::make_error_code(std::errc::not_a_directory),
#ifdef _MSC_VER
	        .excpected = "cov config: generic error: not a directory"sv,
#else
	        .excpected = "cov config: generic error: Not a directory"sv,
#endif
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, error, ::testing::ValuesIn(tests));
}  // namespace cov::app::testing
