// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/tr.hh>
#include <cov/app/strings/cov.hh>

namespace cov::app {
	using covlng = str::root::lng;
	using CovStrings = str::root::Strings;

	template <>
	struct lngs_traits<covlng> : base_lngs_traits<covlng, "cov", CovStrings> {};
}  // namespace cov::app
