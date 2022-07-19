// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/errors.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using errlng = str::errors::lng;
	using ErrorsStrings = str::errors::Strings;

	template <>
	struct lngs_traits<errlng>
	    : base_lngs_traits<errlng, "errors", ErrorsStrings> {};
}  // namespace cov::app
