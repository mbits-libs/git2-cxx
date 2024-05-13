// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cxx-filt/parser.hh>

using namespace std::literals;

namespace cxx_filt {
	std::string_view str_from_token(Token tok) {
		switch (tok) {
			case Token::NONE:
				return ""sv;
			case Token::SPACE:
				return " "sv;
			case Token::COMMA:
				return ","sv;
			case Token::COLON:
				return ":"sv;
			case Token::HASH:
				return "#"sv;
			case Token::SCOPE:
				return "::"sv;
			case Token::PTR:
				return "*"sv;
			case Token::REF:
				return "&"sv;
			case Token::REF_REF:
				return "&&"sv;
			case Token::CONST:
				return " const"sv;
			case Token::VOLATILE:
				return " volatile"sv;
			case Token::OPEN_ANGLE_BRACKET:
				return "<"sv;
			case Token::CLOSE_ANGLE_BRACKET:
				return ">"sv;
			case Token::OPEN_PAREN:
				return "("sv;
			case Token::CLOSE_PAREN:
				return ")"sv;
			case Token::OPEN_CURLY:
				return "{"sv;
			case Token::CLOSE_CURLY:
				return "}"sv;
			case Token::OPEN_BRACKET:
				return "["sv;
			case Token::CLOSE_BRACKET:
				return "]"sv;
		}
		return ""sv;
	}

	std::string_view dbg_from_token(Token tok) {
		switch (tok) {
			case Token::NONE:
				return "<NONE>"sv;
			case Token::SPACE:
				return "<SP>"sv;
			case Token::PTR:
				return "[*]"sv;
			case Token::REF:
				return "[&]"sv;
			case Token::REF_REF:
				return "[&&]"sv;
			case Token::CONST:
				return "<CONST>"sv;
			case Token::VOLATILE:
				return "<VOLATILE>"sv;
			case Token::COMMA:
				return "[,]"sv;
			case Token::COLON:
				return "[:]"sv;
			case Token::HASH:
				return "[#]"sv;
			case Token::SCOPE:
				return "[::]"sv;
			case Token::OPEN_ANGLE_BRACKET:
				return "[<]"sv;
			case Token::CLOSE_ANGLE_BRACKET:
				return "[>]"sv;
			case Token::OPEN_PAREN:
				return "[(]"sv;
			case Token::CLOSE_PAREN:
				return "[)]"sv;
			case Token::OPEN_CURLY:
				return "[{]"sv;
			case Token::CLOSE_CURLY:
				return "[}]"sv;
			case Token::OPEN_BRACKET:
				return "<[>"sv;
			case Token::CLOSE_BRACKET:
				return "<]>"sv;
		}
		return "[???]"sv;
	}
}  // namespace cxx_filt
