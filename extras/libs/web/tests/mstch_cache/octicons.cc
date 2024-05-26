// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
//
// IT IS NATURAL FOR TEST IN HERE TO FAIL FROM TIME TO TIME. IF THEY SWITCH FROM
// SUCCEEDING TO FAILING, A TEST NEEDS TO BE UPDATED TO REFLECT CHANGE IN THE
// REST OF THE CODE.

#include "common.hh"

namespace cov::app::web::testing {
	struct icon_test {
		std::string_view name;
		unsigned width;
		std::string_view path;

		friend std::ostream& operator<<(std::ostream& out,
		                                icon_test const& test) {
			return out << test.name;
		}
	};

	static constexpr icon_test icons[] = {
#include "octicons.inc"
	};

	class octicons : public ::testing::TestWithParam<icon_test> {};
	INSTANTIATE_TEST_SUITE_P(path, octicons, ::testing::ValuesIn(icons));

	TEST(octicons, keys) {
		std::set<std::string_view> expected{};
		for (auto const& icon : icons) {
			expected.insert(icon.name);
		}

		auto const octicons = tests::octicons();
		auto const keys = octicons->debug_all_keys();

		ASSERT_EQ(expected.size(), keys.size());
		for (auto const& key : keys) {
			ASSERT_EQ(1, expected.count(key));
		}
	}

	TEST_P(octicons, svg) {
		auto const& [name, width, path] = GetParam();
		auto const octicons = tests::octicons();
		auto const key = std::string(name);
		ASSERT_TRUE(octicons->has(key));
		auto const actual_node = octicons->at(key);
		ASSERT_TRUE(std::holds_alternative<std::string>(actual_node));
		auto const& actual = std::get<std::string>(actual_node);

		auto const scale = 100 * width / 16;
		auto const size = fmt::format("{}.{:02}em", scale / 100, scale % 100);

		auto const expected = fmt::format(
		    "<svg xmlns='http://www.w3.org/2000/svg' class='octicon' "
		    "width='{size}' height='{size}' viewBox='0 0 {width} "
		    "{width}' "
		    "fill='currentColor'>{path}</svg>",
		    fmt::arg("width", width), fmt::arg("path", path),
		    fmt::arg("size", size));

		ASSERT_EQ(expected, actual);
	}
}  // namespace cov::app::web::testing
