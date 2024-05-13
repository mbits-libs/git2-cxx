// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cxx-filt/expression.hh>
#include <cxx-filt/parser.hh>

using namespace std::literals;

namespace cxx_filt {
	namespace {
		static constexpr std::string_view operators[] = {
		    "..."sv, "<=>"sv, "<<="sv, ">>="sv,  /*"<<"sv, ">>"sv,*/ "<:"sv,
		    ":>"sv,  "<%"sv,  "%>"sv,  "%:%:"sv, "%:"sv,
		    "::"sv,  "->*"sv, "->"sv,  ".*"sv,   "+="sv,
		    "-="sv,  "*="sv,  "/="sv,  "%="sv,   "^="sv,
		    "&="sv,  "|="sv,  "++"sv,  "--"sv,   "=="sv,
		    "!="sv,  "<="sv,  ">="sv,  "&&"sv,   "||"sv,
		    "##"sv,  "<"sv,   ">"sv,   "{"sv,    "}"sv,
		    "["sv,   "]"sv,   "#"sv,   "("sv,    ")"sv,
		    "="sv,   ";"sv,   ":"sv,   "?"sv,    "."sv,
		    "~"sv,   "!"sv,   "+"sv,   "-"sv,    "*"sv,
		    "/"sv,   "%"sv,   "^"sv,   "&"sv,    "|"sv,
		    ","sv,
		};

		bool starts_operator(char c) noexcept {
			switch (c) {
				case '.':
				case '<':
				case '>':
				case ':':
				case '%':
				case '-':
				case '+':
				case '*':
				case '/':
				case '^':
				case '&':
				case '|':
				case '=':
				case '!':
				case '#':
				case '{':
				case '}':
				case '[':
				case ']':
				case '(':
				case ')':
				case ';':
				case '?':
				case '~':
				case ',':
					return true;
			};
			return false;
		};

		bool starts_with(std::string_view self, std::string_view prefix) {
			return std::string_view{self.data(),
			                        std::min(self.size(), prefix.size())} ==
			       prefix;
		}

		std::string str(std::string_view view) {
			return {view.data(), view.size()};
		}
	}  // namespace

	Statement Block::to_statement() const { return to_statement(tokens); }

	ArgumentList Block::to_args() const {
		ArgumentList result{
		    .start = start,
		    .stop = stop,
		};

		size_t prev = 0;

		auto span = vector_view<TokenType const>{tokens};

		for (size_t index = 0; index < span.size(); ++index) {
			if (std::holds_alternative<Token>(span[index]) &&
			    std::get<Token>(span[index]) == Token::COMMA) {
				result.items.push_back(
				    to_statement(span.sub_view(prev, index - prev)));
				prev = index + 1;
			}
		}

		if (prev < span.size()) {
			result.items.push_back(to_statement(span.sub_view(prev)));
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	bool token_is_cvr(Token tok) {
		switch (tok) {
			case Token::CONST:
			case Token::VOLATILE:
			case Token::PTR:
			case Token::REF:
			case Token::REF_REF:
				return true;

			default:
				break;
		}
		return false;
	}

	bool new_expression(Block::TokenType const& prev,
	                    Block::TokenType const& curr) {
		return std::visit(
		    overloaded(
		        [](auto const&, Block const&) { return false; },
		        [](std::string const&, std::string const&) { return true; },
		        [](std::string const&, Token) { return false; },
		        [](Token tok, std::string const&) { return token_is_cvr(tok); },
		        [](Token prev_token, Token current_token) {
			        return token_is_cvr(prev_token) &&
			               !token_is_cvr(current_token);
		        },
		        [](Block const&, std::string const&) { return true; },
		        [](Block const&, Token) { return false; }),
		    prev, curr);
	}

	void append(Expression& target, std::string_view str) {
		if (!target.items.empty() &&
		    std::holds_alternative<std::string>(target.items.back())) {
			std::get<std::string>(target.items.back()).append(str);
			return;
		}

		target.items.push_back(std::string(str.data(), str.size()));
	}

	struct statement_visitor {
		Expression& target;
		CvrLevel& current_level;

		void operator()(std::string const& tok) const {
			if (tok.empty()) return;
			if (tok[0] == '$')
				target.items.push_back(Reference{.id = tok});
			else
				append(target, tok);
		}

		void operator()(Token token) const {
			switch (token) {
				case Token::CONST:
					current_level.has_const = true;
					break;
				case Token::VOLATILE:
					current_level.has_volatile = true;
					break;
				case Token::PTR:
				case Token::REF:
				case Token::REF_REF:
					current_level.level_end = token;
					target.cvrs.levels.push_back(current_level);
					current_level = {};
					break;
				case Token::SPACE:  // GCOV_EXCL_LINE
					break;          // GCOV_EXCL_LINE
				default:
					append(target, str_from_token(token));
					break;
			}
		}

		void operator()(Block const& sub) const {
			if (sub.has_args) {
				target.items.push_back(sub.to_args());
			} else {
				append(target, str_from_token(sub.start));
				for (auto const& token : sub.tokens)
					std::visit(*this, token);
				append(target, str_from_token(sub.stop));
			}
		}
	};

	Statement Block::to_statement(vector_view<TokenType const> tokens) {
		Statement result{};
		CvrLevel current_level{};
		TokenType const* prev = nullptr;

		for (auto const& token : tokens) {
			if (std::holds_alternative<Token>(token) &&
			    std::get<Token>(token) == Token::SPACE)
				continue;
			if (result.items.empty()) {
				result.items.emplace_back();
			} else {
				if (prev && new_expression(*prev, token)) {
					if (current_level.active())
						result.items.back().cvrs.levels.push_back(
						    current_level);
					result.items.emplace_back();
				}
			}
			prev = &token;

			std::visit(statement_visitor{.target = result.items.back(),
			                             .current_level = current_level},
			           token);
		}
		if (current_level.active() && !result.items.empty())
			result.items.back().cvrs.levels.push_back(current_level);
		return result;
	}  // GCOV_EXCL_LINE[GCC]

	std::variant<std::monostate, std::string_view, Token>
	Parser::next() noexcept {
		if (data.empty()) return {};
		while (1) {
			if (std::isspace(static_cast<unsigned char>(data.front()))) {
				while (!data.empty() &&
				       std::isspace(static_cast<unsigned char>(data.front()))) {
					data = data.substr(1);
				}

				return Token::SPACE;
			}

			size_t index = 0;
			while (index < data.size() && !starts_operator(data[index]) &&
			       !std::isspace(static_cast<unsigned char>(data[index]))) {
				++index;
			}

			if (index > 0) {
				auto result = data.substr(0, index);
				data = data.substr(index);
				if (result == "const"sv) return Token::CONST;
				if (result == "volatile"sv) return Token::VOLATILE;
				return result;
			}

			for (auto const op : operators) {
				if (starts_with(data, op)) {
					data = data.substr(op.length());
					if (op == "("sv) return Token::OPEN_PAREN;
					if (op == ")"sv) return Token::CLOSE_PAREN;
					if (op == "{"sv) return Token::OPEN_CURLY;
					if (op == "}"sv) return Token::CLOSE_CURLY;
					if (op == "["sv) return Token::OPEN_BRACKET;
					if (op == "]"sv) return Token::CLOSE_BRACKET;
					if (op == "<"sv) return Token::OPEN_ANGLE_BRACKET;
					if (op == ">"sv) return Token::CLOSE_ANGLE_BRACKET;
					if (op == ","sv) return Token::COMMA;
					if (op == "&"sv) return Token::REF;
					if (op == "&&"sv) return Token::REF_REF;
					if (op == "*"sv) return Token::PTR;
					if (op == "::"sv) return Token::SCOPE;
					if (op == ":"sv) return Token::COLON;
					if (op == "#"sv) return Token::HASH;
					return op;
				}
			}

			// GCOV_EXCL_START
			auto const ret = data.substr(0, 1);
			data = data.substr(1);
			return ret;
		}
		// GCOV_EXCL_STOP
		return {};
	}

	struct TokenStream {
		Parser& self;
		std::variant<std::monostate, std::string_view, Token> tok = self.next();
		std::variant<std::monostate, std::string_view, Token>&
		token() noexcept {
			return tok;
		}

		void next() { tok = self.next(); }

		struct Loop {
			TokenStream& parent;
			bool active{true};
			~Loop() {
				if (active) parent.next();
			}
			void disable() { active = false; }
		};
	};

	Block Parser::next_block(Token start, Token end, bool has_args) {
		Block result{.start = start, .stop = end, .has_args = has_args};
		TokenStream stream{.self = *this};
		while (!std::holds_alternative<std::monostate>(stream.token())) {
			TokenStream::Loop loop{stream};
			if (std::holds_alternative<Token>(stream.token())) {
				auto const token = std::get<Token>(stream.token());
				if (token == end) {
					loop.disable();
					break;
				}

				if (token == Token::OPEN_PAREN) {
					result.tokens.push_back(next_block(
					    Token::OPEN_PAREN, Token::CLOSE_PAREN, true));
					continue;
				}

				if (token == Token::OPEN_ANGLE_BRACKET) {
					result.tokens.push_back(
					    next_block(Token::OPEN_ANGLE_BRACKET,
					               Token::CLOSE_ANGLE_BRACKET, true));
					continue;
				}

				if (token == Token::OPEN_CURLY) {
					result.tokens.push_back(next_block(
					    Token::OPEN_CURLY, Token::CLOSE_CURLY, false));
					continue;
				}

				if (token == Token::OPEN_BRACKET) {
					result.tokens.push_back(next_block(
					    Token::OPEN_BRACKET, Token::CLOSE_BRACKET, false));
					continue;
				}

				/*if (token == Token::SPACE) {
				    if (!result.tokens.empty() &&
				        std::holds_alternative<std::string>(
				            result.tokens.back())) {
				        std::get<std::string>(result.tokens.back())
				            .append(" "sv);
				    }
				    continue;
				}*/

				result.tokens.push_back(token);
				continue;
			}

			auto token = std::get<std::string_view>(stream.token());
			if (!result.tokens.empty() &&
			    std::holds_alternative<std::string>(result.tokens.back())) {
				std::get<std::string>(result.tokens.back()).append(token);
			} else {
				result.tokens.push_back(str(token));
			}
		}
		return result;
	}  // GCOV_EXCL_LINE[GCC]

	Statement Parser::statement_from(std::string_view data) {
		return Parser{.data = data}.block().to_statement();
	}
}  // namespace cxx_filt
