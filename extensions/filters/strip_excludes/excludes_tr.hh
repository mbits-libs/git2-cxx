// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/strip_excludes.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using ExcludesLng = str::excludes::lng;
	using ExcludesCounted = str::excludes::counted;
	using ExcludesStrings = str::excludes::Strings;

	template <>
	struct lngs_traits<ExcludesLng>
	    : base_lngs_traits<ExcludesLng, "strip_excludes", ExcludesStrings> {};
}  // namespace cov::app
