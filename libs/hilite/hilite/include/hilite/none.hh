// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include "hilite/hilite.hh"

namespace hl::none {
	std::string_view token_to_string(unsigned) noexcept;
	void tokenize(const std::string_view& contents, callback& result);
}  // namespace hl::none
