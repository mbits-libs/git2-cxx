// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include <source_location>
#include "../path-utils.hh"
#include "../print-view.hh"

namespace cov::testing {
	struct props_test {
		std::string_view props{};
		struct {
			bool labels{false};
			bool decorate{true};
		} flags{};
		struct {
			std::string_view props{};
			std::string_view simple{};
			std::string_view simple_colors{};
			std::string_view wrapped{};
			std::string_view wrapped_colors{};
		} expected{};
		std::source_location loc{std::source_location::current()};

		friend std::ostream& operator<<(std::ostream& out,
		                                props_test const& test) {
			return print_view(out << std::boolalpha, test.props)
			       << " labels:" << test.flags.labels
			       << " decorate:" << test.flags.decorate;
		}
	};

	class props : public ::testing::TestWithParam<props_test> {
	public:
		void colorize(
		    std::string_view props,
		    std::string_view format,
		    std::string_view expected,
		    bool use_color,
		    decltype(props_test::flags) const& flags,
		    std::source_location const& loc,
		    std::source_location caller = std::source_location::current()) {
			auto const formatter = cov::formatter::from(format);
			auto const builds =
			    cov::report::builder{}.add_nfo({.props = props}).release();

			auto facade = cov::placeholder::object_facade::present_build(
			    builds.front().get(), nullptr);
			cov::placeholder::environment env{
			    .colorize{use_color ? formatter::shell_colorize : nullptr},
			    .decorate{flags.decorate},
			    .prop_names{flags.labels},
			};
			auto const actual = formatter.format(facade.get(), env);

			EXPECT_EQ(expected, actual)
			    << actual << "\n"
			    << caller.file_name() << ':' << caller.line() << "\n"
			    << loc.file_name() << ':' << loc.line();
		}
	};

	TEST_P(props, normalize) {
		auto const& [props, flags, expected, loc] = GetParam();
		EXPECT_EQ(expected.props, cov::report::builder::normalize(props))
		    << loc.file_name() << ':' << loc.line();
	}

	TEST_P(props, format) {
		auto const& [props, flags, expected, loc] = GetParam();
		colorize(props, "%Z", expected.simple, true, flags, loc);
	}

	TEST_P(props, wrapped) {
		auto const& [props, flags, expected, loc] = GetParam();
		colorize(props, "%z", expected.wrapped, true, flags, loc);
	}

	TEST_P(props, colors) {
		auto const& [props, flags, expected, loc] = GetParam();
		colorize(props, "%mZ", expected.simple_colors, true, flags, loc);
	}

	TEST_P(props, wrapped_colors) {
		auto const& [props, flags, expected, loc] = GetParam();
		colorize(props, "%mz", expected.wrapped_colors, true, flags, loc);
	}

	TEST_P(props, colors_disabled) {
		auto const& [props, flags, expected, loc] = GetParam();
		colorize(props, "%mZ", expected.simple, false, flags, loc);
	}

	TEST_P(props, wrapped_colors_disabled) {
		auto const& [props, flags, expected, loc] = GetParam();
		colorize(props, "%mz", expected.wrapped, false, flags, loc);
	}

	static constexpr props_test tests[] = {
	    {},
	    {
	        .props = R"({"name1":"value","name2":false,"name3":314})"sv,
	        .expected{
	            .props = R"("name1":"value","name2":false,"name3":314)"sv,
	            .simple = "value, name2: off, name3: 314"sv,
	            .simple_colors = "\x1B[33mvalue\x1B[m\x1B[2;33m, "
	                             "name2: \x1B[m\x1B[34moff\x1B[m\x1B[2;33m, "
	                             "name3: \x1B[m\x1B[32m314\x1B[m"sv,
	            .wrapped = " (value, name2: off, name3: 314)"sv,
	            .wrapped_colors =
	                "\x1B[2;33m (\x1B[m\x1B[33mvalue\x1B[m\x1B[2;33m, "
	                "name2: \x1B[m\x1B[34moff\x1B[m\x1B[2;33m, "
	                "name3: \x1B[m\x1B[32m314\x1B[m\x1B[2;33m)\x1B[m"sv,
	        },
	    },
	};  // namespace cov::testing
	INSTANTIATE_TEST_SUITE_P(tests, props, ::testing::ValuesIn(tests));
}  // namespace cov::testing
