// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <hilite/lighter.hh>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace cov::app::line_printer {
	std::string to_string(std::optional<unsigned> const& count,
	                      std::string_view view,
	                      lighter::highlighted_line const& items,
	                      std::map<std::uint32_t, std::string> const& dict,
	                      bool use_color,
	                      size_t tab_size = 4);
	std::string to_string(std::optional<unsigned> const& count,
	                      std::string_view view, bool shortened,
	                      bool use_color,
	                      size_t tab_size = 4);
}  // namespace cov::app::line_printer
