// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_log.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using loglng = str::cov_log::lng;
	using LogStrings = str::cov_log::Strings;

	template <>
	struct lngs_traits<loglng>
	    : base_lngs_traits<loglng, "cov_log", LogStrings> {};
}  // namespace cov::app
