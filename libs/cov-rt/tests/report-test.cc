// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/app/report.hh>
#include <json/json.hpp>

namespace cov::testing {
	std::ostream& single_char(std::ostream& out, char c) {
		switch (c) {
			case '"':
				return out << "\\\"";
			case '\n':
				return out << "\\n";
			case '\r':
				return out << "\\r";
			case '\t':
				return out << "\\t";
			case '\v':
				return out << "\\v";
			case '\a':
				return out << "\\a";
			default:
				if (!std::isprint(static_cast<unsigned char>(c))) {
					char buffer[10];
					snprintf(buffer, sizeof(buffer), "\\x%02x",
					         static_cast<unsigned char>(c));
					return out << buffer;
				} else {
					return out << c;
				}
		}
	}

	std::ostream& print_view(std::ostream& out, std::string_view text) {
		if (text.empty()) {
			return out << "{}";
		}
		out << '"';
		for (auto c : text) {
			single_char(out, c);
		}
		return out << '"';
	}
}  // namespace cov::testing

namespace cov::app::report {
	void PrintTo(app::report::report_info const& report, std::ostream* out) {
		using namespace ::std::literals;
		if (report.files.empty() && report.git.head.empty() &&
		    report.git.branch.empty()) {
			*out << "{}"sv;
			return;
		}

		*out << '{';
		if (report.git.head.empty() && report.git.branch.empty()) {
			*out << ".git = {}"sv;
		} else {
			cov::testing::print_view(*out << ".git = {.branch = "sv,
			                         report.git.branch);
			cov::testing::print_view(*out << ", .head = "sv, report.git.head)
			    << '}';
		}

		if (!report.files.empty()) {
			*out << ", .files = {"sv;
			for (auto const& file : report.files) {
				cov::testing::print_view(*out << "{.name = ", file.name);
				cov::testing::print_view(*out << ", .algorithm = "sv, file.algorithm);
				cov::testing::print_view(*out << ", .digest = "sv, file.digest)
				    << ", .line_coverage = {"sv;
				for (auto const [line, hits] : file.line_coverage)
					*out << '{' << line << ',' << hits << "},"sv;
				*out << "}}, "sv;
			}
			*out << '}';
		}

		*out << '}';
	}
}  // namespace cov::app::report

namespace cov::app::testing {
	using namespace ::std::literals;

	struct report_test {
		std::string_view text{};
		app::report::report_info expected{};
		bool succeeds{true};
	};

	class report : public ::testing::TestWithParam<report_test> {};

	TEST_P(report, from_text) {
		auto const& [text, expected, succeeds] = GetParam();
		app::report::report_info actual{};

		if (!text.empty()) {
			auto jsn = json::read_json(
			    {reinterpret_cast<char8_t const*>(text.data()), text.size()});
			ASSERT_NE(nullptr, json::cast<json::map>(jsn));
		}

		auto const result = actual.load_from_text(text);
		ASSERT_EQ(succeeds, result);
		ASSERT_EQ(expected, actual);
	}

	namespace {
		consteval unsigned just_right() {
			return std::numeric_limits<unsigned>::max();
		}

		consteval long long too_much() {
			return static_cast<long long>(just_right()) + 10;
		}

		static char json_buffer[2048];
		static constexpr auto json_prefix =
		    R"({
	"git": {
		"branch": "main",
		"head": "hash"
	},
	"files": [
		{
			"name": "A",
			"digest": "alg:value",
			"line_coverage": {
				"15": 156,
				"16": -5,
				"17": 0,
				"18": )"sv;
		static constexpr auto json_suffix = R"(
			}
		}
	]
})"sv;

		constexpr char* copy(char* dst, char const* src, size_t length) {
			while (length--)
				*dst++ = *src++;
			return dst;
		}

		constexpr std::string_view json_text() {
			char digits[20];
			auto ptr = digits;
			auto value = too_much();
			while (value) {
				auto const digit = value % 10 + '0';
				value /= 10;
				*ptr++ = static_cast<char>(digit);
			}

			auto dst = json_buffer;
			dst = copy(dst, json_prefix.data(), json_prefix.size());
			while (ptr != digits) {
				*dst++ = *--ptr;
			}
			dst = copy(dst, json_suffix.data(), json_suffix.size());
			return {json_buffer, static_cast<size_t>(dst - json_buffer)};
		}
	}  // namespace

	static report_test const good[] = {
	    {
	        .text =
	            R"({"git": {"branch": "main", "head": "hash"}, "files": []})"sv,
	        .expected = {.git = {.branch = "main", .head = "hash"}},
	    },
	    {
	        .text =
	            R"({"git": {"branch": "main", "head": "hash"}, "files": [
	{"name": "A", "digest": "alg:value", "line_coverage": {}}
]})"sv,
	        .expected = {.git = {.branch = "main", .head = "hash"},
	                     .files = {{.name = "A",
	                                .algorithm = "alg", .digest = "value",
	                                .line_coverage = {}}}},
	    },
	    {
	        .text = json_text(),
	        .expected = {.git = {.branch = "main", .head = "hash"},
	                     .files = {{.name = "A",
	                                .algorithm = "alg", .digest = "value",
	                                .line_coverage =
	                                    {
	                                        {15, 156},
	                                        {16, 0},
	                                        {17, 0},
	                                        {18, just_right()},
	                                    }}}},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(good, report, ::testing::ValuesIn(good));

	static report_test const bad[] = {
	    {.succeeds = false},
	    {.text = "{}"sv, .succeeds = false},
	    {.text = R"({"git": {}, "files": []})"sv, .succeeds = false},
	    {.text =
	         R"({"git": {"branch": "main", "head": "hash"}, "files": {}})"sv,
	     .succeeds = false},
	    {.text = R"({"git": {"head": "hash"}, "files": []})"sv,
	     .succeeds = false},
	    {.text = R"({"git": {"branch": "main"}, "files": []})"sv,
	     .succeeds = false},
	    {.text = R"({"git": {"branch": "main"}, "files": [{}]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{"name": "A", "digest": "alg:value", "line_coverage": []}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{"name": "A", "digest": "alg:value"}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{"digest": "alg:value", "line_coverage": {}}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{"name": "A", "line_coverage": {}}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{
		"name": "A",
		"digest": "alg:value",
		"line_coverage": {
			"not-a-number": 0
		}
	}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{
		"name": "A",
		"digest": "alg:value",
		"line_coverage": {
			"15": 1.0
		}
	}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{
		"name": "A",
		"digest": "alg:value",
		"line_coverage": {
			"15": "a string"
		}
	}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{
		"name": "A",
		"digest": "alg, value",
		"line_coverage": {
			"15": "a string"
		}
	}
]})"sv,
	     .succeeds = false},
	};

	INSTANTIATE_TEST_SUITE_P(bad, report, ::testing::ValuesIn(bad));
}  // namespace cov::app::testing
