// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <c++filt/expression.hh>

namespace cxx_filt {
	void append_replacements(std::string_view json_text, Replacements& result);
}  // namespace cxx_filt
