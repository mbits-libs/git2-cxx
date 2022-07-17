// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_init.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using initlng = str::cov_init::lng;
	using IinitStrings = str::cov_init::Strings;

	template <>
	struct lngs_traits<initlng>
	    : base_lngs_traits<initlng, "cov_init", IinitStrings> {};
}  // namespace cov::app
