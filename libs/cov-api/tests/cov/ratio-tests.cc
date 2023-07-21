// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cov/io/types.hh>

using namespace std::literals;

namespace cov::testing {
	using io::v1::stats;
	enum class rel { lt, eq, gt };

	template <typename Int>
	static constexpr stats::ratio<Int> ratio(Int whole,
	                                         Int fraction,
	                                         unsigned char digits) noexcept {
		return {whole, fraction, digits};
	}

	template <typename Int>
	static constexpr stats::ratio<Int> ratio(Int whole) noexcept {
		return {whole, 0, 0};
	}

	template <typename Int>
	struct ratio_test {
		stats::ratio<Int> lhs, rhs;
		rel relation;

		friend inline std::ostream& operator<<(std::ostream& out,
		                                       ratio_test<Int> const& test) {
			out << fmt::format("{} ", test.lhs);
			if (test.relation == rel::lt)
				out << "<";
			else if (test.relation == rel::eq)
				out << "==";
			else if (test.relation == rel::gt)
				out << ">";
			else
				out << "?";
			return out << fmt::format(" {}", test.rhs);
		}
	};

	template <typename Int>
	struct ratio_fmt_test {
		stats::ratio<Int> ratio;
		std::string_view fmt;
		std::string_view expected;

		friend inline std::ostream& operator<<(
		    std::ostream& out,
		    ratio_fmt_test<Int> const& test) {
			return out << fmt::format("{} x \"{}\" => \"{}\"", test.ratio,
			                          test.fmt, test.expected);
		}
	};

	template <typename Int>
	class basic_ratio : public ::testing::TestWithParam<ratio_test<Int>> {
	public:
		void run_lt() {
			auto const& [lhs, rhs, relation] = this->GetParam();
			ASSERT_EQ(lhs < rhs, relation == rel::lt);
		}
		void run_eq() {
			auto const& [lhs, rhs, relation] = this->GetParam();
			ASSERT_EQ(lhs == rhs, relation == rel::eq);
		}
		void run_gt() {
			auto const& [lhs, rhs, relation] = this->GetParam();
			ASSERT_EQ(lhs > rhs, relation == rel::gt);
		}
	};

	template <typename Int>
	class basic_ratio_fmt
	    : public ::testing::TestWithParam<ratio_fmt_test<Int>> {
	public:
		void test() {
			auto const& [ratio, format, expected] = this->GetParam();
			auto const actual = fmt::format(fmt::runtime(format), ratio);
			ASSERT_EQ(expected, actual);
		}
	};

#define RATE_TEST(FIXTURE, OP) \
	TEST_P(FIXTURE, is_##OP) { run_##OP(); }
#define RATE_TESTS(FIXTURE, INT)                          \
	class FIXTURE : public basic_ratio<INT> {};           \
	class FIXTURE##_fmt : public basic_ratio_fmt<INT> {}; \
	RATE_TEST(FIXTURE, lt)                                \
	RATE_TEST(FIXTURE, eq)                                \
	RATE_TEST(FIXTURE, gt)                                \
	TEST_P(FIXTURE##_fmt, format) { test(); }

	RATE_TESTS(u_ratio, unsigned)
	RATE_TESTS(i_ratio, int)

	static constinit ratio_test<unsigned> const u_tests[] = {
	    {ratio(5u, 1u, 1), ratio(5u, 10u, 2), rel::eq},
	    {ratio(5u, 1u, 1), ratio(6u, 10u, 2), rel::lt},
	    {ratio(0u, 9u, 2), ratio(0u, 9u, 3), rel::gt},
	    {ratio(0u, 99u, 3), ratio(0u, 9u, 2), rel::gt},
	    {ratio(0u, 99u, 3), ratio(0u, 99u, 2), rel::lt},
	};

	static constinit ratio_test<int> const i_tests[] = {
	    {ratio(5, 1, 1), ratio(5, 10, 2), rel::eq},
	    {ratio(0, 9, 2), ratio(0, 9, 3), rel::gt},
	    {ratio(5, 1, 1), ratio(6, 10, 2), rel::lt},
	    {ratio(0, 1, 1), ratio(0, -10, 2), rel::gt},
	};

	INSTANTIATE_TEST_SUITE_P(compare, u_ratio, ::testing::ValuesIn(u_tests));
	INSTANTIATE_TEST_SUITE_P(compare, i_ratio, ::testing::ValuesIn(i_tests));

	static constinit ratio_fmt_test<unsigned> const u_fmt_tests[] = {
	    {ratio(5u, 1u, 1), "{}"sv, "5.1"sv},
	    {ratio(5u, 1u, 1), "{:+}"sv, "+5.1"sv},
	    {ratio(5u, 99u, 3), "{:-}"sv, "5.099"sv},
	    {ratio(5u, 1u, 1), "{:+.21}"sv, "+5.1"sv},
	};

	static constinit ratio_fmt_test<int> const i_fmt_tests[] = {
	    {ratio(-5, 1, 1), "{}"sv, "-5.1"sv},
	    {ratio(0, -1, 1), "{}"sv, "-0.1"sv},
	    {ratio(-5, 1, 1), "{:+}"sv, "-5.1"sv},
	    {ratio(0, -1, 1), "{:+}"sv, "-0.1"sv},
	    {ratio(-5, 1, 1), "{:-}"sv, "-5.1"sv},
	    {ratio(0, -1, 1), "{:-}"sv, "-0.1"sv},
	    {ratio(-5, 99, 3), "{:-}"sv, "-5.099"sv},
	    {ratio(2, 10, sizeof(std::uintmax_t) * 8 - 1), "{}"sv,
	     "2.000000000000000000000000000000000000000000000000000000000000010"sv},
	    {ratio(2, 10, sizeof(std::uintmax_t) * 8), "{}"sv,
	     "2.0000000000000000000000000000000000000000000000000000000000000010"sv},
	    {ratio(2, 10, std::numeric_limits<unsigned char>::max()), "{}"sv,
	     "2.0000000000000000000000000000000000000000000000000000000000000000"
	     "000000000000000000000000000000000000000000000000000000000000000000"
	     "000000000000000000000000000000000000000000000000000000000000000000"
	     "00000000000000000000000000000000000000000000000000000000010"sv},
	};

	INSTANTIATE_TEST_SUITE_P(test,
	                         u_ratio_fmt,
	                         ::testing::ValuesIn(u_fmt_tests));
	INSTANTIATE_TEST_SUITE_P(test,
	                         i_ratio_fmt,
	                         ::testing::ValuesIn(i_fmt_tests));
}  // namespace cov::testing
