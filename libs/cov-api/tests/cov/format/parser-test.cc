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
		std::vector<ph::printable> expected{};

		friend std::ostream& operator<<(std::ostream& out,
		                                parser_test const& param) {
			return print_view(out << param.name << ", ", param.tmplt);
		}
	};

	class format_parser : public TestWithParam<parser_test> {};

	struct visit {
		ph::printable const& value;
		struct sub {
			std::ostream& out;

			// std::string, char, color, width, report, commit, person_info
			std::ostream& operator()(std::string const& s) {
				return print_view(out, s);
			}
			std::ostream& operator()(char c) { return print_char(out, c); }
			std::ostream& operator()(ph::color) { return out; }
			std::ostream& operator()(ph::width const& w) {
				return out << "%w(" << w.total << ',' << w.indent1 << ','
				           << w.indent2 << ')';
			}
			std::ostream& operator()(ph::self s) {
				return out << "%self{" << static_cast<int>(s) << '}';
			}
			std::ostream& operator()(ph::stats s) {
				return out << "%stats{" << static_cast<int>(s) << '}';
			}
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

			std::ostream& operator()(ph::block const& loop) {
				out << "%[" << static_cast<int>(loop.type) << ' ' << loop.ref
				    << '{';
				for (auto const& item : loop.opcodes) {
					out << visit{item};
				}
				return out << "}]";
			}
		};
		friend std::ostream& operator<<(std::ostream& out,
		                                visit const& visitor) {
			return std::visit(sub{out}, visitor.value);
		}
	};

	struct print {
		std::vector<ph::printable> const& format;
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

#define MAKE_COLOR_TEST(NAME, COLOR) MAKE_TEST(NAME, "%" NAME, COLOR)

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
	    MAKE_COLOR_TEST("Creset", ph::color::reset),
	    MAKE_COLOR_TEST("Cred", ph::color::red),
	    MAKE_COLOR_TEST("Cgreen", ph::color::green),
	    MAKE_COLOR_TEST("Cblue", ph::color::blue),
	    MAKE_COLOR_TEST("C(normal)", ph::color::normal),
	    MAKE_COLOR_TEST("C(reset)", ph::color::reset),
	    MAKE_COLOR_TEST("C(red)", ph::color::red),
	    MAKE_COLOR_TEST("C(green)", ph::color::green),
	    MAKE_COLOR_TEST("C(yellow)", ph::color::yellow),
	    MAKE_COLOR_TEST("C(blue)", ph::color::blue),
	    MAKE_COLOR_TEST("C(magenta)", ph::color::magenta),
	    MAKE_COLOR_TEST("C(cyan)", ph::color::cyan),
	    MAKE_COLOR_TEST("C(bold normal)", ph::color::bold_normal),
	    MAKE_COLOR_TEST("C(bold red)", ph::color::bold_red),
	    MAKE_COLOR_TEST("C(bold green)", ph::color::bold_green),
	    MAKE_COLOR_TEST("C(bold yellow)", ph::color::bold_yellow),
	    MAKE_COLOR_TEST("C(bold blue)", ph::color::bold_blue),
	    MAKE_COLOR_TEST("C(bold magenta)", ph::color::bold_magenta),
	    MAKE_COLOR_TEST("C(bold cyan)", ph::color::bold_cyan),
	    MAKE_COLOR_TEST("C(bg red)", ph::color::bg_red),
	    MAKE_COLOR_TEST("C(bg green)", ph::color::bg_green),
	    MAKE_COLOR_TEST("C(bg yellow)", ph::color::bg_yellow),
	    MAKE_COLOR_TEST("C(bg blue)", ph::color::bg_blue),
	    MAKE_COLOR_TEST("C(bg magenta)", ph::color::bg_magenta),
	    MAKE_COLOR_TEST("C(bg cyan)", ph::color::bg_cyan),
	    MAKE_COLOR_TEST("C(faint normal)", ph::color::faint_normal),
	    MAKE_COLOR_TEST("C(faint red)", ph::color::faint_red),
	    MAKE_COLOR_TEST("C(faint green)", ph::color::faint_green),
	    MAKE_COLOR_TEST("C(faint yellow)", ph::color::faint_yellow),
	    MAKE_COLOR_TEST("C(faint blue)", ph::color::faint_blue),
	    MAKE_COLOR_TEST("C(faint magenta)", ph::color::faint_magenta),
	    MAKE_COLOR_TEST("C(faint cyan)", ph::color::faint_cyan),
	    MAKE_COLOR_TEST("C(rating)", ph::color::rating),
	    MAKE_COLOR_TEST("C(bold rating)", ph::color::bold_rating),
	    MAKE_COLOR_TEST("C(bg rating)", ph::color::bg_rating),
	    MAKE_COLOR_TEST("C(faint rating)", ph::color::faint_rating),

	    MAKE_TEST("report hash", "%HR", ph::self::primary_hash),
	    MAKE_TEST("contents hash", "%HC", ph::self::primary_hash),
	    MAKE_TEST("primary hash", "%H1", ph::self::primary_hash),

	    MAKE_TEST("file list hash", "%HF", ph::self::secondary_hash),
	    MAKE_TEST("lines hash", "%HL", ph::self::secondary_hash),
	    MAKE_TEST("secondary hash", "%H2", ph::self::secondary_hash),

	    MAKE_TEST("parent hash", "%HP", ph::self::tertiary_hash),
	    MAKE_TEST("functions hash", "%Hf", ph::self::tertiary_hash),
	    MAKE_TEST("tertiary hash", "%H3", ph::self::tertiary_hash),

	    MAKE_TEST("git commit hash", "%HG", ph::self::quaternary_hash),
	    MAKE_TEST("branches hash", "%HB", ph::self::quaternary_hash),
	    MAKE_TEST("quaternary hash", "%H4", ph::self::quaternary_hash),

	    MAKE_TEST("report hash (abbr)", "%hR", ph::self::primary_hash_abbr),
	    MAKE_TEST("contents hash (abbr)", "%hC", ph::self::primary_hash_abbr),
	    MAKE_TEST("primary hash (abbr)", "%h1", ph::self::primary_hash_abbr),

	    MAKE_TEST("file list hash (abbr)",
	              "%hF",
	              ph::self::secondary_hash_abbr),
	    MAKE_TEST("lines hash (abbr)", "%hL", ph::self::secondary_hash_abbr),
	    MAKE_TEST("secondary hash (abbr)",
	              "%h2",
	              ph::self::secondary_hash_abbr),

	    MAKE_TEST("parent hash (abbr)", "%hP", ph::self::tertiary_hash_abbr),
	    MAKE_TEST("functions hash (abbr)", "%hf", ph::self::tertiary_hash_abbr),
	    MAKE_TEST("tertiary hash (abbr)", "%h3", ph::self::tertiary_hash_abbr),

	    MAKE_TEST("git commit hash (abbr)",
	              "%hG",
	              ph::self::quaternary_hash_abbr),
	    MAKE_TEST("branches hash (abbr)",
	              "%hB",
	              ph::self::quaternary_hash_abbr),
	    MAKE_TEST("quaternary hash (abbr)",
	              "%h4",
	              ph::self::quaternary_hash_abbr),

	    MAKE_TEST("refs", "%d", ph::report::ref_names),
	    MAKE_TEST("refs unwrapped", "%D", ph::report::ref_names_unwrapped),
	    MAKE_TEST("magic refs", "%md", ph::report::magic_ref_names),
	    MAKE_TEST("magic refs unwrapped",
	              "%mD",
	              ph::report::magic_ref_names_unwrapped),
	    MAKE_TEST("subject", "%s", ph::commit::subject),
	    MAKE_TEST("sanitized subject", "%f", ph::commit::subject_sanitized),
	    MAKE_TEST("body", "%b", ph::commit::body),
	    MAKE_TEST("raw body", "%B", ph::commit::body_raw),
	    MAKE_TEST("branch", "%rD", ph::report::branch),

	    MAKE_TEST("total text lines", "%pL", ph::stats::lines),
	    MAKE_TEST("lines (percent)", "%pPL", ph::stats::lines_percent),
	    MAKE_TEST("lines (total)", "%pTL", ph::stats::lines_total),
	    MAKE_TEST("lines (visited)", "%pVL", ph::stats::lines_visited),
	    MAKE_TEST("lines (rating)", "%prL", ph::stats::lines_rating),
	    MAKE_TEST("functions (percent)", "%pPF", ph::stats::functions_percent),
	    MAKE_TEST("functions (total)", "%pTF", ph::stats::functions_total),
	    MAKE_TEST("functions (visited)", "%pVF", ph::stats::functions_visited),
	    MAKE_TEST("functions (rating)", "%prF", ph::stats::functions_rating),
	    MAKE_TEST("branches (percent)", "%pPB", ph::stats::branches_percent),
	    MAKE_TEST("branches (total)", "%pTB", ph::stats::branches_total),
	    MAKE_TEST("branches (visited)", "%pVB", ph::stats::branches_visited),
	    MAKE_TEST("branches (rating)", "%prB", ph::stats::branches_rating),

	    MAKE_DATE_TEST("report", "r", ph::who::reporter),
	    MAKE_PERSON_TEST("author", "a", ph::who::author),
	    MAKE_PERSON_TEST("committer", "c", ph::who::committer),

	    MAKE_TEST("empty width", "%w()", (ph::width{76, 6, 9})),
	    MAKE_TEST("width (1 arg)", "%w(60)", (ph::width{60, 6, 9})),
	    MAKE_TEST("width (indent only)", "%w(0, 6)", (ph::width{0, 6, 6})),
	    MAKE_TEST("width (full suite)", "%w(60, 6, 8)", (ph::width{60, 6, 8})),
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
	        "not hex III"sv,
	        "pre %x3\0 post"sv,
	        {"pre %x3\0 post"s},
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
	    {
	        "not direct color (rex)"sv,
	        "pre %Crex post"sv,
	        {"pre %Crex post"s},
	    },
	    {
	        "not direct color (great)"sv,
	        "pre %Cgreat post"sv,
	        {"pre %Cgreat post"s},
	    },
	    {
	        "not direct color (blu)"sv,
	        "pre %Cblu post"sv,
	        {"pre %Cblu post"s},
	    },
	    {
	        "not direct color (zeen)"sv,
	        "pre %Czeen post"sv,
	        {"pre %Czeen post"s},
	    },
	    {
	        "not parens color (bold rex)"sv,
	        "pre %C(bold rex) post"sv,
	        {"pre %C(bold rex) post"s},
	    },
	    {
	        "not parens color (great)"sv,
	        "pre %C(great) post"sv,
	        {"pre %C(great) post"s},
	    },
	    {
	        "not parens color (zeen)"sv,
	        "pre %C(zeen) post"sv,
	        {"pre %C(zeen) post"s},
	    },
	    {
	        "not parens color (aquamarine)"sv,
	        "pre %C(aquamarine) post"sv,
	        {"pre %C(aquamarine) post"s},
	    },
	    {
	        "not width I"sv,
	        "pre %w post"sv,
	        {"pre %w post"s},
	    },
	    {
	        "not width IIa"sv,
	        "pre %w(123,) post"sv,
	        {"pre %w(123,) post"s},
	    },
	    {
	        "not width IIb"sv,
	        "pre %w(123,10,) post"sv,
	        {"pre %w(123,10,) post"s},
	    },
	    {
	        "not width IIc"sv,
	        "pre %w(123,,10) post"sv,
	        {"pre %w(123,,10) post"s},
	    },
	    {
	        "not width IId"sv,
	        "pre %w(,5,10) post"sv,
	        {"pre %w(,5,10) post"s},
	    },
	    {
	        "not width IIIa"sv,
	        "pre %w(123,345,10) post"sv,
	        {"pre %w(123,345,10) post"s},
	    },
	    {
	        "not width IIIb"sv,
	        "pre %w(123,10,345) post"sv,
	        {"pre %w(123,10,345) post"s},
	    },
	    {
	        "not width IV"sv,
	        "pre %w(123,10,11,12) post"sv,
	        {"pre %w(123,10,11,12) post"s},
	    },
	    {
	        "not width Va"sv,
	        "pre %w(123x,10,11) post"sv,
	        {"pre %w(123x,10,11) post"s},
	    },
	    {
	        "not width Vb"sv,
	        "pre %w(123,x,11) post"sv,
	        {"pre %w(123,x,11) post"s},
	    },
	    {
	        "not width Vc"sv,
	        "pre %w(123,10,z) post"sv,
	        {"pre %w(123,10,z) post"s},
	    },
	    {
	        "not magic"sv,
	        "pre %mB post"sv,
	        {"pre %mB post"s},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(bad, format_parser, ::testing::ValuesIn(errors));
}  // namespace cov::testing
