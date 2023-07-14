// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/app/report.hh>
#include <cov/git2/global.hh>
#include <json/json.hpp>
#include "setup.hh"

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
	struct print_digest {
		digest type;
		friend std::ostream& operator<<(std::ostream& out, print_digest value) {
			switch (value.type) {
				case digest::unknown:
					return out << "app::report::digest::unknown"sv;
				case digest::md5:
					return out << "app::report::digest::md5"sv;
				case digest::sha1:
					return out << "app::report::digest::sha1"sv;
			}
			return out << "app::report::digest{"sv
			           << static_cast<int>(value.type) << '}';
		}
	};

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
				*out << ", .algorithm = "sv << print_digest{file.algorithm};
				cov::testing::print_view(*out << ", .digest = "sv, file.digest)
				    << ", .line_coverage = {"sv;
#if defined(__GNUC__)
// Both line and hits are integers of some type. Adding a reference will create
// a need for dereference...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrange-loop-construct"
#endif
				for (auto const [line, hits] : file.line_coverage)
					*out << '{' << line << ',' << hits << "},"sv;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
				*out << "}}, "sv;
			}
			*out << '}';
		}

		*out << '}';
	}

	inline text operator~(text op) {
		using underlying = std::underlying_type_t<text>;
		return static_cast<text>(~static_cast<underlying>(op));
	}

	void PrintTo(text flags, std::ostream* out) {
		using underlying = std::underlying_type_t<text>;
		if (flags == text::missing) {
			*out << "text::missing"sv;
		}
		bool first = true;
#define TEST_FLAG(FLAG)           \
	if ((flags & FLAG) == FLAG) { \
		flags = flags & ~FLAG;    \
		if (first)                \
			first = false;        \
		else                      \
			*out << " | "sv;      \
		*out << #FLAG;            \
	}
		TEST_FLAG(text::in_repo);
		TEST_FLAG(text::in_fs);
		TEST_FLAG(text::different_newline);
		TEST_FLAG(text::mismatched);
		if (flags != text{}) {
			if (first)
				first = false;
			else
				*out << " | "sv;
			*out << "0x"sv << std::hex << static_cast<underlying>(flags);
		}
	}
}  // namespace cov::app::report

namespace cov::io::v1 {
	void PrintTo(coverage const& line, std::ostream* out) {
		*out << "{.value=";
		if (line.value >= 10240) *out << "0x" << std::hex;
		*out << line.value << std::dec << "u, .is_null=" << line.is_null
		     << "u}";
	}
	void PrintTo(coverage_stats const& stats, std::ostream* out) {
		*out << "{.total=" << stats.lines_total
		     << "u, .relevant=" << stats.lines.relevant
		     << "u, .covered=" << stats.lines.visited << "u}";
	}
}  // namespace cov::io::v1

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

	TEST(report, line_coverage) {
		app::report::file_info lines{
		    .line_coverage = {
		        {1, 0},
		        {10, std::numeric_limits<unsigned>::max()},
		        {11, 512},
		        {12, 100},
		    }};
		auto const [actual_coverage, actual_status] =
		    lines.expand_coverage(std::numeric_limits<uint32_t>::max());
		std::vector<io::v1::coverage> expected_coverage{
		    {.value = 0u, .is_null = 0u},
		    {.value = 8u, .is_null = 1u},
		    {.value = 0x7fffffffu, .is_null = 0u},
		    {.value = 512u, .is_null = 0u},
		    {.value = 100u, .is_null = 0u},
		    {.value = 0x7fffffffu, .is_null = 1u},
		    {.value = 0x7ffffff3u, .is_null = 1u}};
		io::v1::coverage_stats expected_status{
		    .lines_total = 4294967295u,
		    .lines = {.relevant = 4u, .visited = 3u}};
		ASSERT_EQ(expected_coverage, actual_coverage);
		ASSERT_EQ(expected_status, actual_status);
	}

	using app::report::text;
	struct verify_test {
		std::string_view title;
		std::string_view filename;
		std::string_view commit;
		std::string_view md5{};
		std::string_view sha1{};
		struct {
			text result{text::missing};
			std::chrono::seconds committed;
			std::string_view message{};
			std::string_view oid{};
			size_t lines{};
		} expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                verify_test const& test) {
			return out << test.filename << "; "sv << test.title;
		}
	};

	class report_verify : public ::testing::TestWithParam<verify_test> {
	protected:
		void verify(std::string_view digest,
		            app::report::digest algorithm,
		            verify_test const& test) {
			git::init globals{};

			auto const& [title, filename, commit_id, _1, _2, expected] = test;
			auto repo = setup::open_verify_repo();
			std::error_code ec{};

			auto const commit =
			    app::report::git_commit::load(repo, commit_id, ec);
			ASSERT_FALSE(ec);

			ASSERT_EQ("Johnny Appleseed"sv, commit.author.name);
			ASSERT_EQ("johnny@appleseed.com"sv, commit.author.mail);
			ASSERT_EQ("Johnny Appleseed"sv, commit.committer.name);
			ASSERT_EQ("johnny@appleseed.com"sv, commit.committer.mail);
			ASSERT_EQ(expected.committed, commit.committed.time_since_epoch());
			ASSERT_EQ(expected.message, commit.message);
			ASSERT_TRUE(commit.tree);

			app::report::file_info file{.algorithm = algorithm};
			file.name.assign(filename);
			file.digest.assign(digest);

			auto const blob = commit.verify(file);

			ASSERT_EQ(expected.result, blob.flags)
			    << "Message: "sv << commit.message;
			if ((expected.result & text::in_repo) == text::in_repo) {
				git_oid id{};
				git_oid_fromstrn(&id, expected.oid.data(), expected.oid.size());
				ASSERT_EQ(0, git_oid_cmp(&id, &blob.existing));
			} else {
				ASSERT_TRUE(git_oid_is_zero(&blob.existing));
			}
			ASSERT_EQ(expected.lines, blob.lines);
		}
	};

	TEST_P(report_verify, md5_upper) {
		std::string digest{};
		digest.assign(GetParam().md5);
		for (auto& c : digest)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		verify(digest, app::report::digest::md5, GetParam());
	}

	TEST_P(report_verify, md5) {
		verify(GetParam().md5, app::report::digest::md5, GetParam());
	}

	TEST_P(report_verify, sha1) {
		verify(GetParam().sha1, app::report::digest::sha1, GetParam());
	}

	TEST_F(report_verify, unknown) {
		static constexpr verify_test const test = {
		    .title{},
		    .filename = "is/unix"sv,
		    .commit = "34c845392a2c508e1a6d7755740485f24f4e19c9"sv,
		    .expected =
		        {
		            // because is/unix is in commit and fs, but unknow digest
		            // algorithm matches nothing
		            .result = text::in_repo | text::in_fs | text::mismatched,
		            .committed = 1659528019s,
		            .message = "commit #1"sv,
		        },
		};
		verify({}, app::report::digest::unknown, test);
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
			"digest": "sha:value",
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
	{"name": "A", "digest": "md5:value", "line_coverage": {}}
]})"sv,
	        .expected = {.git = {.branch = "main", .head = "hash"},
	                     .files = {{.name = "A",
	                                .algorithm = app::report::digest::md5,
	                                .digest = "value",
	                                .line_coverage = {}}}},
	    },
	    {
	        .text =
	            R"({"git": {"branch": "main", "head": "hash"}, "files": [
	{"name": "A", "digest": "sha1:value", "line_coverage": {}}
]})"sv,
	        .expected = {.git = {.branch = "main", .head = "hash"},
	                     .files = {{.name = "A",
	                                .algorithm = app::report::digest::sha1,
	                                .digest = "value",
	                                .line_coverage = {}}}},
	    },
	    {
	        .text = json_text(),
	        .expected = {.git = {.branch = "main", .head = "hash"},
	                     .files = {{.name = "A",
	                                .algorithm = app::report::digest::sha1,
	                                .digest = "value",
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
	{"name": "A", "digest": "md5:value", "line_coverage": []}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{"name": "A", "digest": "md5:value"}
]})"sv,
	     .succeeds = false},
	    {.text = R"({
"git": {"branch": "main", "head": "hash"},
"files": [
	{"digest": "md5:value", "line_coverage": {}}
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
		"digest": "md5:value",
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
		"digest": "md5:value",
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
		"digest": "md5:value",
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
		"digest": "md5, value",
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
		"digest": "md0:value",
		"line_coverage": {
			"15": "a string"
		}
	}
]})"sv,
	     .succeeds = false},
	};

	INSTANTIATE_TEST_SUITE_P(bad, report, ::testing::ValuesIn(bad));

	static verify_test const files[] = {
#include "verify-test.inc"
	};

	INSTANTIATE_TEST_SUITE_P(files, report_verify, ::testing::ValuesIn(files));

	static verify_test const not_hex[] = {
	    {
	        .title = "broken hex, upper nybble"sv,
	        .filename = "is/unix"sv,
	        .commit = "34c845392a2c508e1a6d7755740485f24f4e19c9"sv,
	        .md5 = "f8g44c1e3dbcfd520f09642e0c37d580"sv,
	        .expected =
	            {
	                .result = text::in_repo | text::in_fs | text::mismatched,
	                .committed = 1659528019s,
	                .message = "commit #1"sv,
	            },
	    },
	    {
	        .title = "broken hex, lower nybble"sv,
	        .filename = "is/unix"sv,
	        .commit = "34c845392a2c508e1a6d7755740485f24f4e19c9"sv,
	        .md5 = "f89h4c1e3dbcfd520f09642e0c37d580"sv,
	        .expected =
	            {
	                .result = text::in_repo | text::in_fs | text::mismatched,
	                .committed = 1659528019s,
	                .message = "commit #1"sv,
	            },
	    },
	};

	INSTANTIATE_TEST_SUITE_P(not_hex,
	                         report_verify,
	                         ::testing::ValuesIn(not_hex));
}  // namespace cov::app::testing
