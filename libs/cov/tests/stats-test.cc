// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cov/io/types.hh>
#include <cstdio>
#include <filesystem>
#include "setup.hh"

namespace cov::io::v1 {
	template <typename Unsigned>
	void PrintTo(coverage_stats::ratio<Unsigned> const& ratio,
	             std::ostream* out) {
		if (ratio.digits < 2) {
			*out << fmt::format("{}%", ratio.whole);
			return;
		}
		*out << fmt::format("{}.{:0{}}%", ratio.whole, ratio.fraction,
		                    ratio.digits - 1);
	}
	void PrintTo(coverage_stats const& stats, std::ostream* out) {
		*out << fmt::format("[{}/{} {}]", stats.covered, stats.relevant,
		                    stats.total);
	}
	void PrintTo(coverage_diff const& diff, std::ostream* out) {
		*out << fmt::format("[{}/{} {}]", diff.covered, diff.relevant,
		                    diff.total);
	}
}  // namespace cov::io::v1

namespace cov::testing::setup {
	using namespace ::std::literals;

	template <typename Unsigned>
	struct add_sign {
		using type = std::make_signed_t<Unsigned>;
	};

	template <typename Unsigned>
	struct add_sign<io::v1::coverage_stats::ratio<Unsigned>> {
		using type =
		    io::v1::coverage_stats::ratio<typename add_sign<Unsigned>::type>;
	};

	template <>
	struct add_sign<io::v1::coverage_stats> {
		using type = io::v1::coverage_diff;
	};

	template <typename Unsigned>
	using add_sign_t = typename add_sign<Unsigned>::type;

	template <typename Unsigned>
	struct diff_test {
		Unsigned older;
		Unsigned newer;
		add_sign_t<Unsigned> expected;

		friend void PrintTo(diff_test<Unsigned> const& ratio,
		                    std::ostream* out) {
			PrintTo(ratio.newer, out);
			*out << " - ";
			PrintTo(ratio.older, out);
			*out << " == ";
			PrintTo(ratio.expected, out);
		}
	};

	template <typename Unsigned>
	struct diff_base : ::testing::TestWithParam<diff_test<Unsigned>> {
	protected:
		void run_test() {
			auto const& [older, newer, expected] = this->GetParam();
			auto const actual = io::v1::diff(newer, older);
			if constexpr (std::same_as<Unsigned,
			                           io::v1::coverage_stats::ratio<>>) {
				ASSERT_EQ(expected, actual)
				    << "Value: {" << actual.whole << ", " << actual.fraction
				    << "u, " << actual.digits << "u}";
			} else {
				ASSERT_EQ(expected, actual);
			}
		}
	};

	struct diff_stats : diff_base<io::v1::coverage_stats> {};
	TEST_P(diff_stats, diff) { run_test(); }

	struct diff_ratio : diff_base<io::v1::coverage_stats::ratio<>> {};
	TEST_P(diff_ratio, diff) { run_test(); }

	static constexpr diff_test<io::v1::coverage_stats> stats[] = {
	    {{1000, 300, 299}, {1001, 300, 300}, {+1, 0, +1}},
	    {{9876, 5432, 4321}, {1234, 2345, 6789}, {-8642, -3087, +2468}},
	};

	using stats_t = io::v1::coverage_stats;
	static constexpr diff_test<stats_t::ratio<>> ratios[] = {
	    {
	        stats_t{1000, 300, 299}.calc(4),
	        stats_t{1001, 300, 300}.calc(2),
	        {0, 3333u, 4u},
	    },
	    {
	        stats_t{9876, 5432, 4321}.calc(2),
	        stats_t{1234, 5432, 3321}.calc(2),
	        {-18, 41u, 2u},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, diff_stats, ::testing::ValuesIn(stats));
	INSTANTIATE_TEST_SUITE_P(tests, diff_ratio, ::testing::ValuesIn(ratios));
}  // namespace cov::testing::setup
