// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include "../path-utils.hh"
#include "../print-view.hh"

namespace cov::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	namespace ph = placeholder;

	struct parser_test {
		std::string_view name;
		std::string_view tmplt{};
		std::vector<ph::format> expected{};

		friend std::ostream& operator<<(std::ostream& out,
		                                parser_test const& param) {
			return print_view(out << param.name << ", ", param.tmplt);
		}
	};

	class format_parser : public TestWithParam<parser_test> {};

	struct visit {
		ph::format const& value;
		struct sub {
			std::ostream& out;

			// std::string, char, color, width, report, commit, person_info
			std::ostream& operator()(std::string const& s) {
				return print_view(out, s);
			}
			std::ostream& operator()(char c) { return print_char(out, c); }
			std::ostream& operator()(ph::color) { return out; }
			std::ostream& operator()(ph::width const&) { return out; }
			std::ostream& operator()(ph::report r) {
				return out << "%report{" << static_cast<int>(r) << '}';
			}
			std::ostream& operator()(ph::commit c) {
				return out << "%commit{" << static_cast<int>(c) << '}';
			}
			std::ostream& operator()(ph::person_info const& pair) {
				switch (pair.first) {
					case ph::who::reporter:
						out << "%report/";
						break;
					case ph::who::author:
						out << "%author/";
						break;
					case ph::who::committer:
						out << "%committer/";
						break;
				}
				return out << "fld{" << static_cast<int>(pair.second) << '}';
			}
		};
		friend std::ostream& operator<<(std::ostream& out,
		                                visit const& visitor) {
			return std::visit(sub{out}, visitor.value);
		}
	};

	struct print {
		std::vector<ph::format> const& format;
		friend std::ostream& operator<<(std::ostream& out, print const& param) {
			out << '{';
			auto first = true;
			for (auto const& fmt : param.format) {
				if (first)
					first = false;
				else
					out << ',';
				out << ' ' << visit{fmt};
			}
			return out << " }";
		}
	};

	TEST_P(format_parser, parse) {
		auto const& [_, tmplt, expected] = GetParam();
		auto fmt = formatter::from(tmplt);
		auto const& actual = fmt.parsed();
		ASSERT_EQ(expected, actual) << "Expected: " << print{expected}
		                            << "\nActual:   " << print{actual};
	}

#define MAKE_TEST(NAME, CODE, VALUE) \
	{ NAME##sv, "pre " CODE " post"sv, {"pre "s, VALUE, " post"s}, }

#define PERSON_TEST(TEST_NAME, NAME, CODE, SUBCODE, WHO, VALUE) \
	MAKE_TEST(TEST_NAME " (" NAME ")", "%" CODE SUBCODE,        \
	          (ph::person_info{WHO, ph::person::VALUE}))

#define MAKE_DATE_TEST(NAME, CODE, WHO)                                    \
	PERSON_TEST("date", NAME, CODE, "d", WHO, date),                       \
	    PERSON_TEST("relative date", NAME, CODE, "r", WHO, date_relative), \
	    PERSON_TEST("timestamp", NAME, CODE, "t", WHO, date_timestamp),    \
	    PERSON_TEST("ISO-like", NAME, CODE, "i", WHO, date_iso_like),      \
	    PERSON_TEST("ISO date", NAME, CODE, "I", WHO, date_iso_strict),    \
	    PERSON_TEST("short date", NAME, CODE, "s", WHO, date_short)

#define MAKE_PERSON_TEST(NAME, CODE, WHO)                              \
	PERSON_TEST("name", NAME, CODE, "n", WHO, name),                   \
	    PERSON_TEST("email", NAME, CODE, "e", WHO, email),             \
	    PERSON_TEST("email local", NAME, CODE, "l", WHO, email_local), \
	    MAKE_DATE_TEST(NAME, CODE, WHO)

	static parser_test const tests[] = {
	    {"empty"sv},
	    {
	        "percent"sv,
	        "%%"sv,
	        {'%'},
	    },
	    {
	        "new line"sv,
	        "%n"sv,
	        {'\n'},
	    },
	    {
	        "hex"sv,
	        "pre%x20post"sv,
	        {"pre"s, ' ', "post"s},
	    },
	    {
	        "hex"sv,
	        "pre %xAb post"sv,
	        {"pre "s, '\xab', " post"s},
	    },
	    MAKE_TEST("refs", "%d", ph::report::ref_names),
	    MAKE_TEST("refs unwrapped", "%D", ph::report::ref_names_unwrapped),
	    MAKE_TEST("subject", "%s", ph::commit::subject),
	    MAKE_TEST("sanitized subject", "%f", ph::commit::subject_sanitized),
	    MAKE_TEST("body", "%b", ph::commit::body),
	    MAKE_TEST("raw body", "%B", ph::commit::body_raw),
	    MAKE_TEST("report hash", "%Hr", ph::report::hash),
	    MAKE_TEST("commit hash", "%Hc", ph::commit::hash),
	    MAKE_TEST("report hash (abbr)", "%hr", ph::report::hash_abbr),
	    MAKE_TEST("commit hash (abbr)", "%hc", ph::commit::hash_abbr),
	    MAKE_TEST("parent hash", "%rP", ph::report::parent_hash),
	    MAKE_TEST("parent hash (abbr)", "%rp", ph::report::parent_hash_abbr),
	    MAKE_TEST("branch", "%rD", ph::report::branch),
	    MAKE_TEST("percent", "%pP", ph::report::lines_percent),
	    MAKE_TEST("total", "%pT", ph::report::lines_total),
	    MAKE_TEST("relevant", "%pR", ph::report::lines_relevant),
	    MAKE_TEST("covered", "%pC", ph::report::lines_covered),
	    MAKE_TEST("covered", "%pr", ph::report::lines_rating),

	    MAKE_DATE_TEST("report", "r", ph::who::reporter),
	    MAKE_PERSON_TEST("author", "a", ph::who::author),
	    MAKE_PERSON_TEST("committer", "c", ph::who::committer),
	};

	INSTANTIATE_TEST_SUITE_P(good, format_parser, ::testing::ValuesIn(tests));

	static parser_test const errors[] = {
	    {
	        "not recognized"sv,
	        "pre %z post"sv,
	        {"pre %z post"s},
	    },
	    {
	        "not hex I"sv,
	        "pre %x0g post"sv,
	        {"pre %x0g post"s},
	    },
	    {
	        "not hex II"sv,
	        "pre %xhg post"sv,
	        {"pre %xhg post"s},
	    },
	    {
	        "not hash"sv,
	        "pre %Ha post"sv,
	        {"pre %Ha post"s},
	    },
	    {
	        "not abbr hash"sv,
	        "pre %ha post"sv,
	        {"pre %ha post"s},
	    },
	    {
	        "not report"sv,
	        "pre %rz post"sv,
	        {"pre %rz post"s},
	    },
	    {
	        "not author"sv,
	        "pre %az post"sv,
	        {"pre %az post"s},
	    },
	    {
	        "not committer"sv,
	        "pre %cz post"sv,
	        {"pre %cz post"s},
	    },
	    {
	        "not line coverage"sv,
	        "pre %pz post"sv,
	        {"pre %pz post"s},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(bad, format_parser, ::testing::ValuesIn(errors));
}  // namespace cov::testing
