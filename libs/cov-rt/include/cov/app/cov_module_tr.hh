// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_module.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using modlng = str::cov_module::lng;
	using modcnt = str::cov_module::counted;
	using ModuleStrings = str::cov_module::Strings;

	template <>
	struct lngs_traits<modlng>
	    : base_lngs_traits<modlng, "cov_module", ModuleStrings> {};
}  // namespace cov::app
