// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_show.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using showlng = str::cov_show::lng;
	using ShowStrings = str::cov_show::Strings;

	template <>
	struct lngs_traits<showlng>
	    : base_lngs_traits<showlng, "cov_show", ShowStrings> {};
}  // namespace cov::app
