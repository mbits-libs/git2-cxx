// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <gtest/gtest.h>
#include <cov/app/args.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/log_format_tr.hh>
#include <cov/app/rt_path.hh>
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
		struct {
			std::string_view en{};
			std::string_view pl{};
		} expected{};
	};

	class error : public ::testing::TestWithParam<error_test> {};

	TEST(error, args) {
		git::init globals{};
		std::vector<std::string> en{"en"};
		str::args_translator<> tr{
		    {platform::sys_root() / directory_info::share / "locale"sv, en}};

		using lng = str::args::lng;
		using parser = base_parser<>;
		using args_description = parser::args_description;
		using visitor = parser::string::str_visitor;

		static constexpr auto descr_00 =
		    args_description{"-a, --arg"sv, lng::HELP_DESCRIPTION,
		                     lng::FILE_META, lng::DEF_META};
		static constexpr auto descr_01 =
		    args_description{"-a, --arg"sv, lng::HELP_DESCRIPTION,
		                     opt{lng::FILE_META}, lng::DEF_META};
		static constexpr auto descr_10 =
		    args_description{"-a, --arg"sv, lng::HELP_DESCRIPTION,
		                     lng::FILE_META, opt{lng::DEF_META}};
		static constexpr auto descr_11 =
		    args_description{"-a, --arg"sv, lng::HELP_DESCRIPTION,
		                     opt{lng::FILE_META}, opt{lng::DEF_META}};
		static constexpr auto descr_0 = args_description{
		    "-a, --arg"sv, lng::HELP_DESCRIPTION, lng::FILE_META};
		static constexpr auto descr_1 = args_description{
		    "-a, --arg"sv, lng::HELP_DESCRIPTION, opt{lng::FILE_META}};

		visitor V{tr};
#define ASSERT_EQUAL(FIRST, SECOND, DESCR)          \
	do {                                            \
		auto const [first, second] = DESCR.with(V); \
		ASSERT_EQ(FIRST, first);                    \
		ASSERT_EQ(SECOND, second);                  \
	} while (0)
		ASSERT_EQUAL("-a, --arg <file> <arg>"sv,
		             "shows this help message and exits"sv, descr_00);
		ASSERT_EQUAL("-a, --arg [<file>] <arg>"sv,
		             "shows this help message and exits"sv, descr_01);
		ASSERT_EQUAL("-a, --arg <file> [<arg>]"sv,
		             "shows this help message and exits"sv, descr_10);
		ASSERT_EQUAL("-a, --arg [<file>] [<arg>]"sv,
		             "shows this help message and exits"sv, descr_11);
		ASSERT_EQUAL("-a, --arg <file>"sv,
		             "shows this help message and exits"sv, descr_0);
		ASSERT_EQUAL("-a, --arg [<file>]"sv,
		             "shows this help message and exits"sv, descr_1);
	}

	TEST_P(error, message_en) {
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
		ASSERT_EQ(expected.en, actual);
	}

	TEST_P(error, message_pl) {
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

		std::vector<std::string> pl{"pl"};
		str::args_translator<errlng> tr{
		    {platform::sys_root() / directory_info::share / "locale"sv, pl}};

		parser_holder holder{{"cov config"sv, {}}, {}, tr.args()};
		auto const actual = holder.message(copy, tr);
		ASSERT_EQ(expected.pl, actual);
	}

	struct LogBuiltin : lngs::storage::Builtin<str::log_format::Resource> {
		std::string_view operator()(str::log_format::lng id) const noexcept {
			return get_string(static_cast<identifier>(id));
		}
	};

	TEST(error, load_builtin_log_messages) {
		using namespace str::log_format;
		LogBuiltin builtin{};

		ASSERT_TRUE(builtin.init_builtin());
		// "ïñ ŧĥê ƒũŧũȓê"
		ASSERT_EQ(
		    "\xC3\xAF\xC3\xB1 \xC5\xA7\xC4\xA5\xC3\xAA \xC6\x92\xC5\xA9\xC5\xA7\xC5\xA9\xC8\x93\xC3\xAA"sv,
		    builtin(lng::IN_THE_FUTURE));
	}

	static error_test const tests[] = {
	    {
	        .git = {GIT_ERROR_REGEX, "error message goes here"},
	        .expected =
	            {.en = "cov config: regex error: error message goes here"sv,
	             .pl = "cov config: b\xC5\x82\xC4\x85"
	                   "d regex: error message goes here"sv},
	    },
	    {
	        .git = {GIT_ERROR_CONFIG, "error message goes here"},
	        .expected = {.en = "cov config: error: error message goes here"sv,
	                     .pl = "cov config: b\xC5\x82\xC4\x85"
	                           "d: error message goes here"sv},
	    },
	    {
	        .git = {GIT_ERROR_INDEX, "neither regex nor config"},
	        .expected = {.en = "cov config: error: neither regex nor config"sv,
	                     .pl = "cov config: b\xC5\x82\xC4\x85"
	                           "d: neither regex nor config"sv},
	    },
	    {
	        .ec = std::make_error_code(std::errc::not_a_directory),
#ifdef _MSC_VER
	        .expected = {.en = "cov config: generic error: not a directory"sv,
	                     .pl = "cov config: b\xC5\x82\xC4\x85"
	                           "d generic: not a directory"sv},
#else
	        .expected = {.en = "cov config: generic error: Not a directory"sv,
	                     .pl = "cov config: b\xC5\x82\xC4\x85"
	                           "d generic: Not a directory"sv},
#endif
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, error, ::testing::ValuesIn(tests));
}  // namespace cov::app::testing
