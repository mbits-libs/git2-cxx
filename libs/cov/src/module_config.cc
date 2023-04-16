// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/file.hh>
#include <cov/module.hh>
#include "cell/ascii.hh"
#include "cell/character.hh"
#include "cell/context.hh"
#include "cell/operators.hh"
#include "cell/repeat_operators.hh"
#include "cell/special.hh"

using namespace std::literals;

#define RULE_MAP(name)                                                      \
	struct on_##name##_handler {                                            \
		constexpr on_##name##_handler() = default;                          \
		template <class Context>                                            \
		void operator()(Context& context) const;                            \
	};                                                                      \
	static constexpr auto on_##name = on_##name##_handler{};                \
	template <class Context>                                                \
	void on_##name##_handler::operator()([[maybe_unused]] Context& context) \
	    const

#define RULE_APPEND(name) \
	RULE_MAP(name) { _append(context, token::name); }

namespace cov::parser {
	using namespace cell;

	enum class token { header, value, other };

	template <typename Context>
	void _append(Context& context, token kind) {
		_val(context).append(context, kind);
	}

	template <typename Context>
	void _eol(Context& context) {
		_val(context).eol(context);
	}

	RULE_APPEND(header);
	RULE_APPEND(value);
	RULE_APPEND(other);
	RULE_MAP(eol) { _val(context).eol(context); }

	RULE_MAP(comment) { _val(context).comment(context); }

	struct dbg {
		std::string_view tag;
		void operator()([[maybe_unused]] auto& context) const {
#if 0
			auto const view = _view(context);
			printf("%.*s [%.*s]\n", static_cast<int>(tag.size()), tag.data(),
			       static_cast<int>(view.size()), view.data());
#endif
		}
	};

	static constexpr auto SP = *inlspace;
	static constexpr auto non_eol = ch - eol;

	static constexpr auto namechar = alnum | '-';
	static constexpr auto header_char = namechar | '.';
	static constexpr auto name = (alnum >> *namechar)[dbg("IDENT"sv)];
	static constexpr auto header_ident =
	    (alnum >> *header_char)[dbg("IDENT"sv)];

	static constexpr auto unescaped = ch - ch("\"\\");
	static constexpr auto escaped = '\\' >> ch;
	static constexpr auto string =
	    ('"' >> *(unescaped | escaped) >> '"')[dbg("STR"sv)];

	static constexpr auto unspaced = ch - ch(" \t\r\n\";#");
	static constexpr auto value_token = (SP >> (+unspaced | string));

	static constexpr auto comment = (SP >> -(ch(";#") >> *non_eol))[on_comment];

	static constexpr auto header =
	    (SP >> '[' >> SP >> header_ident >> -(SP >> string) >> SP >>
	     ']')[dbg("HDR"sv)] >>
	    comment;
	static constexpr auto value =
	    (SP >> name >> SP >> '=' >> SP >> *value_token)[dbg("VAR"sv)] >>
	    comment;

	static constexpr auto line =
	    header[on_header] | value[on_value] | comment[on_other];

	static constexpr auto config =
	    *(ahead(line >> eol) >> line >> eol[on_eol]) | -line;

	struct range_t {
		size_t start;
		size_t stop;
	};
	struct token_t {
		range_t line;
		range_t comment;
		token tok;
		bool use{true};

		void print(std::string_view view, io::file& out) {
			auto const start = line.start;
			auto const stop = line.stop;
			if (comment.start == comment.stop) {
				auto const chunk = view.substr(start, stop - start);
				out.store(chunk.data(), chunk.size());
				return;
			}
			auto const chunk1 = view.substr(start, comment.start - start);
			auto const chunk2 = view.substr(comment.stop, stop - comment.stop);
			out.store(chunk1.data(), chunk1.size());
			out.store(chunk2.data(), chunk2.size());
		}
	};

	class lines_type {
	public:
		std::vector<token_t> lines{};
		range_t comm{0, 0};
		void append(size_t start, size_t stop, token tok) {
			lines.push_back({{start, stop}, comm, tok});
			comm = {0, 0};
		}
		void eol(size_t stop) { lines.back().line.stop = stop; }
		void comment(size_t start, size_t stop) {
			comm = {start, stop};
			if (start == stop) return;
		}

	private:
	};

	template <typename Iterator>
	class grammar_value {
		lines_type* ref_{nullptr};

	public:
		Iterator begin_{};

		grammar_value() = default;
		grammar_value(lines_type* ref, Iterator begin)
		    : ref_{ref}, begin_{begin} {}

		template <typename Context>
		void append(Context& context, token kind) {
			auto const start =
			    static_cast<size_t>(std::distance(begin_, context.range.first));
			auto const stop = static_cast<size_t>(
			    std::distance(begin_, context.range.second));
			ref_->append(start, stop, kind);
		}
		template <typename Context>
		void eol(Context& context) {
			auto const stop = static_cast<size_t>(
			    std::distance(begin_, context.range.second));
			ref_->eol(stop);
		}

		template <typename Context>
		void comment(Context& context) {
			auto const start =
			    static_cast<size_t>(std::distance(begin_, context.range.first));
			auto const stop = static_cast<size_t>(
			    std::distance(begin_, context.range.second));
			ref_->comment(start, stop);
		}
	};

	template <typename Iterator>
	void parse(Iterator begin, Iterator end, lines_type& result) {
		using value_t = grammar_value<Iterator>;
		using context_t = cell::context<Iterator, epsilon, value_t>;

		value_t val{&result, begin};
		auto ctx = context_t{eps, val};
		config.parse(begin, end, ctx);
	}
}  // namespace cov::parser

namespace cov {
	void modules::cleanup_config(std::filesystem::path const& filename) {
		// TODO: needs safe file

		auto file = io::fopen(filename);
		if (!file) return;

		auto bytes = file.read();
		auto view = std::string_view{
		    reinterpret_cast<char const*>(bytes.data()), bytes.size()};

		file = io::fopen(filename, "w");
		if (!file) return;

		parser::lines_type text{};
		parser::parse(view.begin(), view.end(), text);

		size_t prev_header = 0;
		auto seen_value = false;
		auto const size = text.lines.size();
		for (size_t index = 0; index < size; ++index) {
			auto const& line = text.lines[index];
			if (line.tok == parser::token::header) {
				prev_header = index;
				seen_value = false;
				continue;
			}

			if (line.tok == parser::token::value) {
				if (!seen_value) {
					seen_value = true;
					text.lines[prev_header].print(view, file);
				}

				text.lines[index].print(view, file);
				continue;
			}
		}  // GCOV_EXCL_LINE
	}
}  // namespace cov
