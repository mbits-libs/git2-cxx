// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_config.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using cfglng = str::cov_config::lng;
	using ConfigStrings = str::cov_config::Strings;

	template <>
	struct lngs_traits<cfglng>
	    : base_lngs_traits<cfglng, "cov_config", ConfigStrings> {};
}  // namespace cov::app
