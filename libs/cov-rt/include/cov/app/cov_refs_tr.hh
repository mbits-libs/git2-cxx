// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_refs.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using refslng = str::cov_refs::lng;
	using RefsStrings = str::cov_refs::Strings;

	template <>
	struct lngs_traits<refslng>
	    : base_lngs_traits<refslng, "cov_refs", RefsStrings> {};
}  // namespace cov::app
