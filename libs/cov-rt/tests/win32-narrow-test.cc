// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cctype>
#include <cov/app/dirs.hh>

namespace cov::app::testing {
	using namespace ::std::literals;
	struct test_category : public std::error_category {
		std::string msg;
		test_category(std::string_view native)
		    : msg{native.data(), native.size()} {}
		const char* name() const noexcept override { return "test category"; }
		std::string message(int) const override { return msg; }

		inline std::error_code make() const {
			return std::error_code(1, *this);
		}
	};

	struct test {
		unsigned int cp;
		std::string_view native{};
		std::u8string_view expected{};
	};

	class narrow : public ::testing::TestWithParam<test> {};

	TEST_P(narrow, check) {
		auto const& [cp, native, u8expected] = GetParam();

		std::string_view expected{
		    reinterpret_cast<char const*>(u8expected.data()),
		    u8expected.size()};
#ifdef _WIN32
		test_category mock{native};
		auto actual = platform::con_to_u8(mock.make(), cp);
#else
		test_category mock{expected};
		auto actual = platform::con_to_u8(mock.make());
#endif
		ASSERT_EQ(expected, actual);
	}

	static constexpr test tests[] = {
	    {1250u, "\261\200\300\340\212\252\271\352\320"sv, u8"±€ŔŕŠŞąęĐ"sv},
	    {1253u, "\341\342\343\344\345"sv, u8"αβγδε"sv},
	};
	INSTANTIATE_TEST_SUITE_P(tests, narrow, ::testing::ValuesIn(tests));
}  // namespace cov::app::testing
