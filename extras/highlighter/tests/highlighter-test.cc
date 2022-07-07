// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "support.hh"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <limits>

#ifdef __has_include
#if __has_include("hilite/cxx.hh")
#define HAS_CXX 1
#endif
#if __has_include("hilite/py3.hh")
#define HAS_PY3 1
#endif
#endif

namespace highlighter::testing {
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct hilite_ctx {
		std::string_view hl_macro{};
		std::string_view span_prefix{};
	};

	struct hilite_test {
		std::string_view name;
		std::string_view text;
		hilite_ctx ctx;
		highlights expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                hilite_test const& test) {
			return print_view(out, test.name);
		}
	};

	class highlighter : public TestWithParam<hilite_test> {};

	TEST_P(highlighter, colors) {
		auto const& [name, text, ctx, expected] = GetParam();
		printer_state::setup(ctx.hl_macro, ctx.span_prefix);
		auto actual = highlights::from(text, name);
		printer_state::get().dict = &actual.dict;
		ASSERT_EQ(expected, actual);

		if (!actual.lines.empty() && actual.lines.back().contents.empty()) {
			actual.lines.pop_back();
		}

		std::cout << ">> " << name << ":\n";
		for (auto const& line : actual.lines) {
			auto const length = length_of(line.contents);
			auto const view = text.substr(line.start, length);
			std::string_view color;
			std::cout << print{line.contents, view, color, actual.dict} << '\n';
		}
	}

	static constexpr hilite_ctx none_ctx{{}, "hl"sv};

	static const hilite_test none[] = {
	    {
	        ".hidden-cc"sv,
	        R"(#include <iostream>

int main() {
    std::cout << "Hello, World!\n";
})"sv,
	        none_ctx,
	        {
	            .dict{},
	            .lines{
	                hl_line{0u, {text_span{0u, 19u}}},
	                hl_line{20u, {}},
	                hl_line{21u, {text_span{0u, 12u}}},
	                hl_line{34u, {text_span{0u, 35u}}},
	                hl_line{70u, {text_span{0u, 1u}}},
	            },
	        },
	    },
	    {
	        "CRLF"sv,
	        "#include <iostream>\r\n"
	        "\r\n"
	        "int main() {\r\n"
	        "    std::cout << \"Hello, World!\\n\";\r\n"
	        "}",
	        none_ctx,
	        {
	            .dict{},
	            .lines{
	                hl_line{0u, {text_span{0u, 19u}}},
	                hl_line{21u, {}},
	                hl_line{23u, {text_span{0u, 12u}}},
	                hl_line{37u, {text_span{0u, 35u}}},
	                hl_line{74u, {text_span{0u, 1u}}},
	            },
	        },
	    },
	    {
	        "CR-only"sv,
	        "#include <iostream>\r"
	        "\r"
	        "int main() {\r"
	        "    std::cout << \"Hello, World!\\n\";\r"
	        "}",
	        none_ctx,
	        {
	            .dict{},
	            .lines{
	                hl_line{0u, {text_span{0u, 19u}}},
	                hl_line{20u, {}},
	                hl_line{21u, {text_span{0u, 12u}}},
	                hl_line{34u, {text_span{0u, 35u}}},
	                hl_line{70u, {text_span{0u, 1u}}},
	            },
	        },
	    },
	};

	INSTANTIATE_TEST_SUITE_P(none, highlighter, ::testing::ValuesIn(none));
}  // namespace highlighter::testing

#ifdef HAS_CXX
#include "highlighter-cxx.hh"
#endif

#ifdef HAS_PY3
#include "highlighter-py3.hh"
#endif
