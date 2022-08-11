// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <args/parser.hpp>
#include <cov/format_args.hh>

namespace cov::testing {
	using namespace ::std::literals;

	TEST(use_feature, ctors) {
		color_feature clr;
		decorate_feature decorate;
		ASSERT_EQ(clr, use_feature::automatic);
		ASSERT_EQ(decorate, use_feature::automatic);
	}

	TEST(use_feature, color) {
		using converter = ::args::converter<color_feature>;
		args::null_translator tr{};
		args::parser p{{}, args::args_view{}, &tr};
		ASSERT_EQ(use_feature::yes, converter::value(p, "always"s, {}));
		ASSERT_EQ(use_feature::no, converter::value(p, "never"s, {}));
		ASSERT_EQ(use_feature::automatic, converter::value(p, "auto"s, {}));
	}

	TEST(use_feature, decorate) {
		using converter = ::args::converter<decorate_feature>;
		args::null_translator tr{};
		args::parser p{{}, args::args_view{}, &tr};
		ASSERT_EQ(use_feature::yes, converter::value(p, "short"s, {}));
		ASSERT_EQ(use_feature::no, converter::value(p, "no"s, {}));
		ASSERT_EQ(use_feature::automatic, converter::value(p, "auto"s, {}));
	}
}  // namespace cov::testing
