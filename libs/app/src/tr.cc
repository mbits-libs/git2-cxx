// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/tr.hh>

namespace cov::app::str {
	std::string args_strings::args(::args::lng id,
	                               std::string_view arg1,
	                               std::string_view arg2) const {
		using enum ::args::lng;
		using str::args::lng;

		auto const ident = [](auto id) {
			static constexpr std::pair<::args::lng, lng> lng_map[] = {
			    {usage, lng::USAGE},
			    {def_meta, lng::DEF_META},
			    {positionals, lng::POSITIONALS},
			    {optionals, lng::OPTIONALS},
			    {help_description, lng::HELP_DESCRIPTION},
			    {unrecognized, lng::UNRECOGNIZED},
			    {needs_param, lng::NEEDS_PARAM},
			    {needs_no_param, lng::NEEDS_NO_PARAM},
			    {needs_number, lng::NEEDS_NUMBER},
			    {needed_number_exceeded, lng::NEEDED_NUMBER_EXCEEDED},
			    {needed_enum_unknown, lng::NEEDS_ENUM_UNKNOWN},
			    {needed_enum_known_values, lng::NEEDS_ENUM_KNOWN_VALUES},
			    {required, lng::REQUIRED},
			    {error_msg, lng::ERROR_MSG},
			    {file_not_found, lng::FILE_NOT_FOUND},
			};
			for (auto [from, to] : lng_map) {
				if (from == id) return to;
			}
			[[unlikely]];                // GCOV_EXCL_LINE
			return static_cast<lng>(0);  // GCOV_EXCL_LINE
		}(id);
		return fmt::format(fmt::runtime((*this)(ident)), arg1, arg2);
	}
}  // namespace cov::app::str
