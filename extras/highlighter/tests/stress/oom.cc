// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include "../support.hh"
#include "new.hh"

#include "cell/ascii.hh"
#include "cell/character.hh"
#include "cell/operators.hh"
#include "cell/repeat_operators.hh"
#include "cell/special.hh"
#include "hilite/hilite_impl.hh"

namespace hl::words {
	using namespace cell;

	RULE_EMIT(newline, token::newline)
	RULE_EMIT(word, token::identifier)

	constexpr auto SP = *inlspace;
	constexpr auto WRD = alnum | punct;
	constexpr auto EOL = eol[on_newline];
	constexpr auto word = +WRD[on_word];
	constexpr auto text = *(SP >> word | EOL);

	void tokenize(const std::string_view& contents, callback& result) {
		auto begin = contents.begin();
		const auto end = contents.end();
		auto value = grammar_result{};

		parse_with_restart<grammar_value>(begin, end, text, empty, value);
		value.produce_lines(result, contents.size());
	}
}  // namespace hl::words

namespace highlighter::testing {
	using namespace ::std::literals;

	struct callback : hl::callback {
		void on_line(std::size_t, std::size_t, const hl::tokens&) final {}
	};

	TEST(oom, break_lines) {
		callback cb{};
		OOM_LIMIT(16384);
		hl::words::tokenize(
		    R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc nec est
tristique, molestie erat nec, aliquet turpis. Phasellus tristique
molestie nisi, ac pretium quam dictum at.)"sv,
		    cb);
		OOM_END;
	}

	TEST(oom, node_spans) {
		callback cb{};
		OOM_LIMIT(724);
		highlights::from(
		    R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc nec est
tristique, molestie erat nec, aliquet turpis. Phasellus tristique
molestie nisi, ac pretium quam dictum at.)"sv,
		    ""sv);
		OOM_END;
	}
}  // namespace highlighter::testing
