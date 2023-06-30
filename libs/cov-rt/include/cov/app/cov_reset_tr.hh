// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_reset.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using resetlng = str::cov_reset::lng;
	using ResetStrings = str::cov_reset::Strings;

	template <>
	struct lngs_traits<resetlng>
	    : base_lngs_traits<resetlng, "cov_reset", ResetStrings> {};
}  // namespace cov::app
