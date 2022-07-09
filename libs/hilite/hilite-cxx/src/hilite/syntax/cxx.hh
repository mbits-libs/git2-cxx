// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include "hilite/hilite.hh"

#define CXX_TOKENS(X)                                  \
	X(deleted_newline) /*slash followed by a newline*/ \
	X(local_header_name)                               \
	X(system_header_name)                              \
	X(universal_character_name)                        \
	X(macro_name)                                      \
	X(macro_arg_list)                                  \
	X(macro_arg)                                       \
	X(macro_va_args)                                   \
	X(macro_replacement)

#define CXX_HILITE_TOKENS(X) \
	HILITE_TOKENS(X)         \
	CXX_TOKENS(X)

namespace hl::cxx {
	// something happens here, and CXX_TOKENS get indended and preproc even so
	// clang-format off
	enum token : unsigned {
#define LIST_TOKENS(x) x = hl::token::x,
		HILITE_TOKENS(LIST_TOKENS)
#undef LIST_TOKENS
#define LIST_TOKENS(x) x,
		CXX_TOKENS(LIST_TOKENS)
#undef LIST_TOKENS
		preproc = meta,
		preproc_identifier = meta_identifier,
		known_ident_op_replacement = known_ident_1,
		known_ident_cstdint = known_ident_2,
		known_ident_stl = known_ident_3,
	};
	// clang-format on

	std::string_view token_to_string(unsigned) noexcept;
	void tokenize(const std::string_view& contents, callback& result);
}  // namespace hl::cxx
