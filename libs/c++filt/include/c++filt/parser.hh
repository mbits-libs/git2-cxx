// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <c++filt/types.hh>

namespace cxx_filt {
	struct Block;
	struct Statement;
	struct Expression;
	struct ArgumentList;

	struct Block {
		using TokenType = std::variant<std::string, Token, Block>;

		Token start;
		Token stop;
		std::vector<TokenType> tokens{};
		bool has_args;

		Statement to_statement() const;
		ArgumentList to_args() const;

	private:
		static Statement to_statement(vector_view<TokenType const> tokens);
	};

	struct Parser {
		std::string_view data;

		std::variant<std::monostate, std::string_view, Token> next() noexcept;

		Block next_block(Token start, Token end, bool has_args);
		Block block() { return next_block({}, {}, false); }

		static Statement statement_from(std::string_view data);
	};

}  // namespace cxx_filt
